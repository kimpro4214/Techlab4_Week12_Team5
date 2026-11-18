#include "EnginePCH.h"
#include "ParticleEmittersPanel.h"
#include "Curves/CurveFloat.h"
#include "Editor/ParticleEditor/ParticleEditor.h"
#include "Render/Particle/ParticleHelper.h"
#include "Render/Particle/ParticleSystem.h"
#include "Render/Particle/ParticleModuleCollision.h"
#include "Render/Particle/ParticleModuleColor.h"
#include "Render/Particle/ParticleModuleColorOverLife.h"
#include "Render/Particle/ParticleModuleLifetime.h"
#include "Render/Particle/ParticleModuleLocation.h"
#include "Render/Particle/ParticleModuleSize.h"
#include "Render/Particle/ParticleModuleSizeMultiplyLife.h"
#include "Render/Particle/ParticleModuleVelocity.h"

FParticleEmittersPanel::FParticleEmittersPanel(FParticleEditor& InEditor)
	: FParticleEditorPanel(InEditor, "Emitters")
{
}

void FParticleEmittersPanel::DrawEmitterName(int32 Index, UParticleEmitter& Emitter)
{
	if (RenamingEmitter == &Emitter)
	{
		if (bFocusRename)
		{
			ImGui::SetKeyboardFocusHere();
			bFocusRename = false;
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		const bool bConfirmed = ImGui::InputText("##EmitterName", RenameBuffer.data(), RenameBuffer.size(),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		if ((bConfirmed || ImGui::IsItemDeactivatedAfterEdit()) && !ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			FString Name = RenameBuffer.data();
			const size_t First = Name.find_first_not_of(" \t");
			if (First != FString::npos)
			{
				Emitter.SetEmitterName(Name.substr(First, Name.find_last_not_of(" \t") - First + 1));
			}
		}
		if (bConfirmed || ImGui::IsItemDeactivated())
		{
			RenamingEmitter = nullptr;
		}
		return;
	}

	const FString Label = Editor.GetEmitterDisplayName(Index);
	const ImVec2 TextPosition = ImGui::GetCursorScreenPos();
	if (ImGui::Selectable("##EmitterNameLabel",
		Editor.GetSelectedEmitterIndex() == Index && !Editor.GetSelectedModule() && !Editor.IsTypeDataSelected(),
		ImGuiSelectableFlags_AllowDoubleClick))
	{
		Editor.SelectEmitter(Index);
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			RenamingEmitter = &Emitter;
			RenameBuffer.assign(std::max<size_t>(256, Label.size() + 1), '\0');
			std::copy(Label.begin(), Label.end(), RenameBuffer.begin());
			bFocusRename = true;
		}
	}
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	DrawList->PushClipRect(TextPosition, ImGui::GetItemRectMax(), true);
	DrawList->AddText(TextPosition, ImGui::GetColorU32(ImGuiCol_Text), Label.c_str());
	DrawList->PopClipRect();
}

bool FParticleEmittersPanel::DrawEmitterMenu(int32 Index, bool bHasLOD)
{
	bool bRemoveEmitter = false;
	if (ImGui::BeginPopupContextWindow("EmitterContext"))
	{
		if (ImGui::BeginMenu("Add Module", bHasLOD))
		{
			const struct
			{
				const char* Label;
				UClass* (*GetClass)();
			} ModuleTypes[] = {
				{ "Lifetime", UParticleModuleLifetime::StaticClass },
				{ "Location", UParticleModuleLocation::StaticClass },
				{ "Velocity", UParticleModuleVelocity::StaticClass },
				{ "Color", UParticleModuleColor::StaticClass },
				{ "Color Over Life", UParticleModuleColorOverLife::StaticClass },
				{ "Size", UParticleModuleSize::StaticClass },
				{ "Size Multiply Life", UParticleModuleSizeMultiplyLife::StaticClass },
				{ "Collision", UParticleModuleCollision::StaticClass },
			};
			for (const auto& Type : ModuleTypes)
			{
				if (ImGui::MenuItem(Type.Label))
				{
					Editor.AddModule(Index, Type.GetClass());
				}
			}
			ImGui::EndMenu();
		}
		bRemoveEmitter = ImGui::MenuItem("Remove Emitter");
		ImGui::EndPopup();
	}
	return bRemoveEmitter;
}

void FParticleEmittersPanel::DrawContent()
{
	UParticleSystem* System = Editor.GetSystem();
	if (!System) return;

	if (RenameSystem != System)
	{
		RenameSystem = System;
		RenamingEmitter = nullptr;
	}

	if (System->GetEmitters().IsEmpty())
	{
		ImGui::TextUnformatted("No emitters");
		return;
	}

	// 이미터별 열 너비를 유지하고 화면을 넘는 열은 가로 스크롤로 탐색합니다.
	ImGui::BeginChild("EmitterColumns", ImVec2(0, 0), ImGuiChildFlags_None,
		ImGuiWindowFlags_HorizontalScrollbar);
	int32 RemoveIndex = -1;
	int32 ModuleEmitterIndex = -1;
	UParticleModule* ModuleToRemove = nullptr;
	const ImGuiStyle& Style = ImGui::GetStyle();
	const float ControlsWidth = 2.0f * (ImGui::GetFrameHeight() + Style.ItemInnerSpacing.x)
		+ ImGui::CalcTextSize("Enabled").x + ImGui::CalcTextSize("Solo").x + Style.ItemSpacing.x;
	const float ColumnWidth = std::max(220.0f, ControlsWidth + 2.0f * (Style.WindowPadding.x + Style.ScrollbarSize));
	for (int32 Index = 0; Index < System->GetEmitters().Num(); ++Index)
	{
		UParticleEmitter* Emitter = System->GetEmitters()[Index];
		if (!Emitter) continue;

		if (Index > 0) ImGui::SameLine();
		// 각 이미터의 고유 ID로 독립적인 자식 창을 엽니다.
		const FString ColumnId = "Emitter_" + std::to_string(Emitter->GetUUID());
		ImGui::BeginChild(ColumnId.c_str(), ImVec2(ColumnWidth, -1.0f), ImGuiChildFlags_Borders);

		DrawEmitterName(Index, *Emitter);
		const UParticleLODLevel* LOD = Emitter->GetLODLevel(Editor.GetSelectedLODLevel());
		ImGui::BeginDisabled(!LOD);
		bool bEnabled = LOD && LOD->IsEnabled();
		if (ImGui::Checkbox("Enabled", &bEnabled))
		{
			Editor.SetEmitterEnabled(Index, bEnabled);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		bool bSolo = Editor.GetSoloEmitter() == Emitter;
		ImGui::BeginDisabled(!LOD && !bSolo);
		if (ImGui::Checkbox("Solo", &bSolo))
		{
			Editor.SetSoloEmitter(bSolo ? Emitter : nullptr);
		}
		ImGui::EndDisabled();
		ImGui::Separator();

		bool bModuleMenuOpen = false;
		if (LOD)
		{
			auto DrawModule = [&](UParticleModule* Module, const char* Name, bool bRemovable)
			{
				if (!Module) return;
				ImGui::PushID(Module);
				if (ImGui::Selectable(Name, Editor.GetSelectedModule() == Module))
				{
					Editor.SelectModule(Index, Module);
				}
				if (bRemovable && ImGui::BeginPopupContextItem("ModuleContext"))
				{
					bModuleMenuOpen = true;
					if (ImGui::MenuItem("Remove Module"))
					{
						ModuleEmitterIndex = Index;
						ModuleToRemove = Module;
					}
					ImGui::EndPopup();
				}
				ImGui::PopID();
			};

			DrawModule(LOD->GetRequiredModule(), "Required", false);
			DrawModule(LOD->GetSpawnModule(), "Spawn", false);

			const bool bTypeDataSelected = Editor.GetSelectedEmitterIndex() == Index && Editor.IsTypeDataSelected();
			if (ImGui::Selectable("Type Data", bTypeDataSelected))
			{
				Editor.SelectTypeData(Index);
			}

			ImGui::Separator();
			for (UParticleModule* Module : LOD->GetModules())
			{
				if (!Module) continue;
				FString Name = Module->GetClass()->Name;
				const FString Prefix = "UParticleModule";
				if (Name.starts_with(Prefix)) Name.erase(0, Prefix.size());
				if (!Module->IsEnabled()) Name += " (Disabled)";
				DrawModule(Module, Name.c_str(), true);
			}
		}
		else
		{
			ImGui::TextDisabled("LOD unavailable");
		}

		if (!bModuleMenuOpen && DrawEmitterMenu(Index, LOD != nullptr))
		{
			RemoveIndex = Index;
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();

	if (RemoveIndex >= 0)
	{
		if (RenamingEmitter == System->GetEmitters()[RemoveIndex])
		{
			RenamingEmitter = nullptr;
		}
		Editor.RemoveEmitter(RemoveIndex);
	}
	else if (ModuleToRemove)
	{
		Editor.RemoveModule(ModuleEmitterIndex, ModuleToRemove);
	}
}
