#include "EnginePCH.h"
#include "ParticleDetailsPanel.h"
#include "Editor/ParticleEditor/ParticleEditor.h"
#include "Editor/Details/PropertyEditor.h"
#include "Render/Particle/ParticleSystem.h"
#include "UObject/Property.h"
#include "Curves/CurveFloat.h"
#include "UObject/UObjectIterator.h"

FParticleDetailsPanel::FParticleDetailsPanel(FParticleEditor& InEditor)
	: FParticleEditorPanel(InEditor, "Details")
{
}

void FParticleDetailsPanel::DrawContent()
{
	UParticleModule* Module = Editor.GetSelectedModule();
	if (!Module)
	{
		if (UParticleEmitter* Emitter = Editor.GetSelectedEmitter())
		{
			ImGui::TextUnformatted(Editor.GetEmitterDisplayName(Editor.GetSelectedEmitterIndex()).c_str());
			ImGui::Text("LODs: %d", Emitter->GetLODLevels().Num());
		}
		ImGui::TextWrapped("Select a module to edit its properties.");
		return;
	}

	ImGui::TextUnformatted(Module->GetClass()->Name.c_str());
	ImGui::Separator();
	bool bChanged = false;
	for (UClass* Class = Module->GetClass(); Class; Class = Class->Super)
	{
		for (const FProperty& Property : Class->GetProperties())
		{
			if (!Property.HasAnyFlags(CPF_Edit)) continue;
			if (Property.Name == "bEnabled" &&
				(Cast<UParticleModuleRequired>(Module) || Cast<UParticleModuleSpawn>(Module))) continue;
			ImGui::PushID(Property.Name.c_str());

			// 공유 재질의 텍스처를 수정하지 않고 모듈의 재질 참조만 교체합니다.
			if (Property.Type == EPropertyType::Object)
			{
				if (auto* Required = Cast<UParticleModuleRequired>(Module);
					Required && Property.Class == UMaterial::StaticClass())
				{
					UMaterial* Current = Required->GetMaterial();
					const char* Preview = Current ? Current->GetPath().c_str() : "None";
					if (ImGui::BeginCombo("Material", Preview))
					{
						for (TObjectIterator<UMaterial> Material; Material; ++Material)
						{
							if (Material->GetPath().empty() ||
								(Material->PSOType != EPSOType::ParticleSprite_Additive &&
								 Material->PSOType != EPSOType::ParticleSprite_AlphaBlend)) continue;
							if (ImGui::Selectable(Material->GetPath().c_str(), Current == *Material))
							{
								Required->SetMaterial(*Material);
								bChanged = true;
							}
						}
						ImGui::EndCombo();
					}
				}
				else
				{
					UObject* Reference = *reinterpret_cast<UObject**>(reinterpret_cast<uint8*>(Module) + Property.Offset);
					if (const auto* Curve = Cast<UCurveFloat>(Reference))
					{
						ImGui::TextDisabled("%s: %d keys (read-only)", Property.Name.c_str(), Curve->GetKeys().Num());
					}
					else
					{
						ImGui::TextDisabled("%s: None", Property.Name.c_str());
					}
				}
			}
			else
			{
				bChanged |= FPropertyEditor::EditProperty(Module, Property);
			}

			ImGui::PopID();
		}
	}

	if (bChanged) Editor.RefreshSystem();
	if (!Editor.GetLastError().empty())
	{
		ImGui::TextWrapped("%s", Editor.GetLastError().c_str());
	}
}
