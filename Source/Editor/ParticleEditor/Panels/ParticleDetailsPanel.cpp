#include "EnginePCH.h"
#include "ParticleDetailsPanel.h"
#include "Editor/ParticleEditor/ParticleEditor.h"
#include "Editor/Details/PropertyEditor.h"
#include "Render/Particle/ParticleSystem.h"
#include "Render/Particle/ParticleModuleTypeDataBeam.h"
#include "Render/Particle/ParticleModuleTypeDataMesh.h"
#include "Render/Particle/ParticleModuleTypeDataRibbon.h"
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
	if (Editor.IsTypeDataSelected())
	{
		const struct
		{
			const char* Label;
			UClass* Class;
		} Types[] = {
			{ "Sprite", nullptr },
			{ "Mesh", UParticleModuleTypeDataMesh::StaticClass() },
			{ "Ribbon", UParticleModuleTypeDataRibbon::StaticClass() },
			{ "Beam", UParticleModuleTypeDataBeam::StaticClass() },
		};

		UClass* CurrentClass = Module ? Module->GetClass() : nullptr;
		const char* CurrentLabel = CurrentClass ? CurrentClass->Name.c_str() : "Sprite";
		for (const auto& Type : Types)
		{
			if (Type.Class == CurrentClass) CurrentLabel = Type.Label;
		}

		if (ImGui::BeginCombo("Type Data", CurrentLabel))
		{
			for (const auto& Type : Types)
			{
				if (ImGui::Selectable(Type.Label, Type.Class == CurrentClass))
				{
					Editor.SetEmitterTypeData(Editor.GetSelectedEmitterIndex(), Type.Class);
				}
			}
			ImGui::EndCombo();
		}

		Module = Editor.GetSelectedModule();
		if (!Module) return;
	}
	else if (!Module)
	{
		if (UParticleEmitter* Emitter = Editor.GetSelectedEmitter())
		{
			ImGui::TextUnformatted(Editor.GetEmitterDisplayName(Editor.GetSelectedEmitterIndex()).c_str());
			ImGui::Text("LODs: %d", Emitter->GetLODLevels().Num());
		}
		ImGui::TextWrapped("Select a module to edit its properties.");
		return;
	}

	if (!Editor.IsTypeDataSelected())
	{
		ImGui::TextUnformatted(Module->GetClass()->Name.c_str());
	}
	ImGui::Separator();
	bool bChanged = false;
	for (UClass* Class = Module->GetClass(); Class; Class = Class->Super)
	{
		for (const FProperty& Property : Class->GetProperties())
		{
			if (!Property.HasAnyFlags(CPF_Edit)) continue;
			if (Property.Name == "bEnabled" &&
				(Cast<UParticleModuleRequired>(Module) || Cast<UParticleModuleSpawn>(Module) ||
				 Cast<UParticleModuleTypeDataBase>(Module))) continue;
			ImGui::PushID(Property.Name.c_str());

			// 공유 재질의 텍스처를 수정하지 않고 모듈의 재질 참조만 교체합니다.
			if (Property.Type == EPropertyType::Object)
			{
				if (auto* Required = Cast<UParticleModuleRequired>(Module);
					Required && Property.Class == UMaterial::StaticClass())
				{
					const auto* LOD = Cast<UParticleLODLevel>(Required->GetOuter());
					const auto* TypeData = LOD ? LOD->GetTypeDataModule() : nullptr;
					const bool bUsesStrip = Cast<UParticleModuleTypeDataBeam>(TypeData) ||
						Cast<UParticleModuleTypeDataRibbon>(TypeData);
					const bool bUsesMesh = Cast<UParticleModuleTypeDataMesh>(TypeData) != nullptr;
					UMaterial* Current = Required->GetMaterial();
					const char* Preview = Current ? Current->GetPath().c_str() : "None";
					if (ImGui::BeginCombo("Material", Preview))
					{
						for (TObjectIterator<UMaterial> Material; Material; ++Material)
						{
							const EPSOType PSO = Material->PSOType;
							const bool bIsStrip = PSO == EPSOType::ParticleStrip_Additive ||
								PSO == EPSOType::ParticleStrip_AlphaBlend;
							const bool bIsSprite = PSO == EPSOType::ParticleSprite_Additive ||
								PSO == EPSOType::ParticleSprite_AlphaBlend;
							const bool bCompatible = bUsesStrip ? bIsStrip : bIsSprite || (bUsesMesh && PSO == EPSOType::ParticleMesh);
							if (Material->GetPath().empty() || !bCompatible) continue;
							if (ImGui::Selectable(Material->GetPath().c_str(), Current == *Material))
							{
								Required->SetMaterial(*Material);
								bChanged = true;
							}
						}
						ImGui::EndCombo();
					}
				}
				else if (auto* MeshData = Cast<UParticleModuleTypeDataMesh>(Module);
					MeshData && Property.Name == "Mesh")
				{
					UStaticMesh* Current = MeshData->Mesh;
					const char* Preview = Current ? Current->GetPath().c_str() : "None";
					if (ImGui::BeginCombo("Mesh", Preview))
					{
						if (ImGui::Selectable("None", !Current) && Current)
						{
							MeshData->Mesh = nullptr;
							bChanged = true;
						}

						for (TObjectIterator<UStaticMesh> Mesh; Mesh; ++Mesh)
						{
							if (Mesh->GetPath().empty()) continue;
							if (ImGui::Selectable(Mesh->GetPath().c_str(), Current == *Mesh) && Current != *Mesh)
							{
								MeshData->Mesh = *Mesh;
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
