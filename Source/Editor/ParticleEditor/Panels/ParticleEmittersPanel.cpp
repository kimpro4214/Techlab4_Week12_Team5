#include "EnginePCH.h"
#include "ParticleEmittersPanel.h"
#include "Editor/ParticleEditor/ParticleEditor.h"
#include "Render/Particle/ParticleSystem.h"

FParticleEmittersPanel::FParticleEmittersPanel(FParticleEditor& InEditor)
	: FParticleEditorPanel(InEditor, "Emitters")
{
}

void FParticleEmittersPanel::DrawContent()
{
	UParticleSystem* System = Editor.GetSystem();
	if (!System) return;

	ImGui::TextDisabled("LOD 0");
	if (System->GetEmitters().IsEmpty())
	{
		ImGui::TextUnformatted("No emitters");
		return;
	}

	// 이미터별 열 너비를 유지하고 화면을 넘는 열은 가로 스크롤로 탐색합니다.
	ImGui::BeginChild("EmitterColumns", ImVec2(0, 0), ImGuiChildFlags_None,
		ImGuiWindowFlags_HorizontalScrollbar);
	for (int32 Index = 0; Index < System->GetEmitters().Num(); ++Index)
	{
		if (Index > 0) ImGui::SameLine();
		ImGui::PushID(Index);
		ImGui::BeginChild("Emitter", ImVec2(220.0f, -1.0f), ImGuiChildFlags_Borders);

		const FString Label = "Emitter " + std::to_string(Index + 1);
		if (ImGui::Selectable(Label.c_str(),
			Editor.GetSelectedEmitterIndex() == Index && !Editor.GetSelectedModule()))
		{
			Editor.SelectEmitter(Index);
		}
		ImGui::Separator();

		const UParticleEmitter* Emitter = System->GetEmitters()[Index];
		const UParticleLODLevel* LOD = Emitter ? Emitter->GetLODLevel(0) : nullptr;
		if (LOD)
		{
			auto DrawModule = [&](UParticleModule* Module, const char* Name)
			{
				if (!Module) return;
				ImGui::PushID(Module);
				if (ImGui::Selectable(Name, Editor.GetSelectedModule() == Module))
				{
					Editor.SelectModule(Index, Module);
				}
				ImGui::PopID();
			};

			DrawModule(LOD->GetRequiredModule(), "Required");
			DrawModule(LOD->GetSpawnModule(), "Spawn");
			ImGui::Separator();
			for (UParticleModule* Module : LOD->GetModules())
			{
				if (!Module) continue;
				FString Name = Module->GetClass()->Name;
				const FString Prefix = "UParticleModule";
				if (Name.starts_with(Prefix)) Name.erase(0, Prefix.size());
				if (!Module->IsEnabled()) Name += " (Disabled)";
				DrawModule(Module, Name.c_str());
			}
		}

		ImGui::EndChild();
		ImGui::PopID();
	}
	ImGui::EndChild();
}
