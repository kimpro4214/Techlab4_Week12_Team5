#include "EnginePCH.h"
#include "ParticleEditorTab.h"
#include "ParticleEditor.h"
#include "Panels/ParticleViewportPanel.h"
#include "Panels/ParticleDetailsPanel.h"
#include "Panels/ParticleEmittersPanel.h"
#include "Render/Particle/ParticleSystem.h"

namespace
{
enum class EPlaybackIcon
{
	Play,
	Pause,
	Restart,
	Step
};

bool DrawPlaybackButton(const char* Id, const char* Tooltip, EPlaybackIcon Icon)
{
	const float Size = ImGui::GetFrameHeight();
	const bool bPressed = ImGui::Button(Id, ImVec2(Size, Size));
	const ImVec2 Min = ImGui::GetItemRectMin();
	const float X = Min.x + Size * 0.5f;
	const float Y = Min.y + Size * 0.5f;
	const float R = Size * 0.23f;
	const ImU32 Color = ImGui::GetColorU32(ImGuiCol_Text);
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	switch (Icon)
	{
	case EPlaybackIcon::Play:
		DrawList->AddTriangleFilled({X - R, Y - R}, {X - R, Y + R}, {X + R, Y}, Color);
		break;
	case EPlaybackIcon::Pause:
		DrawList->AddRectFilled({X - R, Y - R}, {X - R * 0.35f, Y + R}, Color);
		DrawList->AddRectFilled({X + R * 0.35f, Y - R}, {X + R, Y + R}, Color);
		break;
	case EPlaybackIcon::Restart:
		DrawList->PathArcTo({X, Y}, R, 0.0f, IM_PI * 1.5f);
		DrawList->PathStroke(Color, ImDrawFlags_None, 1.5f);
		DrawList->AddTriangleFilled({X, Y - R * 1.6f}, {X, Y - R * 0.4f}, {X + R, Y - R}, Color);
		break;
	case EPlaybackIcon::Step:
		DrawList->AddTriangleFilled({X - R, Y - R}, {X - R, Y + R}, {X + R * 0.35f, Y}, Color);
		DrawList->AddRectFilled({X + R * 0.65f, Y - R}, {X + R, Y + R}, Color);
		break;
	}

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("%s", Tooltip);
	return bPressed;
}

class FParticleSettingsPanel : public FParticleEditorPanel
{
public:
	explicit FParticleSettingsPanel(FParticleEditor& InEditor)
		: FParticleEditorPanel(InEditor, "Settings")
	{
	}

protected:
	void DrawContent() override
	{
		const bool bPaused = Editor.IsPaused();
		if (DrawPlaybackButton("##PlayPause", bPaused ? "Play" : "Pause",
			bPaused ? EPlaybackIcon::Play : EPlaybackIcon::Pause))
			Editor.SetPaused(!bPaused);
		ImGui::SameLine();
		if (DrawPlaybackButton("##Restart", "Restart", EPlaybackIcon::Restart)) Editor.Restart();
		ImGui::SameLine();
		ImGui::BeginDisabled(!Editor.IsPaused());
		if (DrawPlaybackButton("##Step", "Step", EPlaybackIcon::Step)) Editor.Step();
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::SetNextItemWidth(-FLT_MIN);
		float TimeScale = Editor.GetTimeScale();
		if (ImGui::SliderFloat("##Speed", &TimeScale, 0.1f, 4.0f, "Speed %.1fx")) Editor.SetTimeScale(TimeScale);

		ImGui::Separator();
		bool bGrid = Editor.IsRenderGrid();
		if (ImGui::Checkbox("Grid", &bGrid)) Editor.SetRenderGrid(bGrid);
		ImGui::Text("Particles: %d", Editor.GetActiveParticleCount());
		if (Editor.IsDirty()) ImGui::TextDisabled("Unsaved changes");
		if (!Editor.GetLastError().empty()) ImGui::TextWrapped("%s", Editor.GetLastError().c_str());

		if (UParticleSystem* System = Editor.GetSystem())
		{
			ImGui::SeparatorText("LOD Distances");
			const int32 LODCount = Editor.GetLODLevelCount();
			for (int32 Index = 0; Index < LODCount; ++Index)
			{
				const FString Label = "LOD " + std::to_string(Index);
				float Distance = System->GetLODDistance(Index);
				ImGui::SetNextItemWidth(100.0f);
				ImGui::BeginDisabled(Index == 0);
				if (ImGui::DragFloat(Label.c_str(), &Distance, 0.1f, 0.0f, 0.0f, "%.2f"))
					System->SetLODDistance(Index, Distance);
				ImGui::EndDisabled();
			}
		}
	}
};
}

FParticleEditorTab::FParticleEditorTab(FParticleEditor& InEditor)
	: FAssetEditorTab(InEditor)
	, ParticleEditor(InEditor)
{
	// 탭의 부모 창이 달라져도 애셋별 도킹 ID를 유지합니다.
	const ImGuiID AssetId = ImHashStr(InEditor.GetKey().GetTabId().c_str());
	DockSpaceId = ImHashStr("ParticleDockSpaceV3", 0, AssetId);
	PanelClassId = ImHashStr("ParticlePanelClass", 0, AssetId);

	Panels.Add(MakeUnique<FParticleViewportPanel>(InEditor));
	Panels.Add(MakeUnique<FParticleSettingsPanel>(InEditor));
	Panels.Add(MakeUnique<FParticleDetailsPanel>(InEditor));
	Panels.Add(MakeUnique<FParticleEmittersPanel>(InEditor));
	for (auto& Panel : Panels)
	{
		Panel->SetDockClass(PanelClassId);
		Panel->Init();
	}
}

void FParticleEditorTab::DrawMenuBar()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Save", "Ctrl+S", false, ParticleEditor.CanSave()))
		{
			ParticleEditor.Save();
		}
		if (ImGui::MenuItem("Close Editor"))
		{
			ParticleEditor.Close();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		for (auto& Panel : Panels)
		{
			if (ImGui::MenuItem(Panel->GetPanelName(), nullptr, Panel->IsOpen()))
			{
				Panel->SetOpen(!Panel->IsOpen());
			}
		}
		ImGui::EndMenu();
	}
}

void FParticleEditorTab::DrawToolbar()
{
	const float Padding = ImGui::GetStyle().ItemSpacing.x;
	ImGui::Indent(Padding);

	if (ImGui::Button("Add Emitter"))
	{
		ParticleEditor.AddEmitter();
	}

	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("LOD Level");
	ImGui::SameLine();

	const int32 CurrentLOD = ParticleEditor.GetSelectedLODLevel();
	const int32 LODCount = ParticleEditor.GetLODLevelCount();
	const FString CurrentLabel = "LOD " + std::to_string(CurrentLOD);
	ImGui::BeginDisabled(LODCount <= 1);
	ImGui::SetNextItemWidth(ImGui::CalcTextSize("LOD 0000").x + ImGui::GetFrameHeight());
	if (ImGui::BeginCombo("##LODLevel", CurrentLabel.c_str()))
	{
		for (int32 Index = 0; Index < LODCount; ++Index)
		{
			const FString Label = "LOD " + std::to_string(Index);
			if (ImGui::Selectable(Label.c_str(), Index == CurrentLOD))
			{
				ParticleEditor.SelectLODLevel(Index);
			}
			if (Index == CurrentLOD)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(LODCount == 0);
	if (ImGui::Button("Add LOD Level"))
	{
		ParticleEditor.AddLODLevel();
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::BeginDisabled(LODCount <= 1);
	if (ImGui::Button("Delete Current LOD Level"))
	{
		ParticleEditor.DeleteCurrentLODLevel();
	}
	ImGui::EndDisabled();
	ImGui::Unindent(Padding);
	ImGui::Separator();
}

void FParticleEditorTab::BuildDefaultLayout(ImGuiID InDockSpaceId, const ImVec2& Size)
{
	ImGui::DockBuilderAddNode(InDockSpaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(InDockSpaceId, Size);
	ImGuiID EmittersId = InDockSpaceId;
	ImGuiID LeftId = ImGui::DockBuilderSplitNode(EmittersId, ImGuiDir_Left, 0.25f, nullptr, &EmittersId);
	const ImGuiID ViewportId = ImGui::DockBuilderSplitNode(LeftId, ImGuiDir_Up, 0.4f, nullptr, &LeftId);

	// 남은 높이의 1/3을 Settings에 할당해 전체 높이의 20%로 맞춥니다.
	const ImGuiID SettingsId = ImGui::DockBuilderSplitNode(LeftId, ImGuiDir_Up, 1.0f / 3.0f, nullptr, &LeftId);

	ImGui::DockBuilderDockWindow(Panels[0]->GetWindowName(), ViewportId);
	ImGui::DockBuilderDockWindow(Panels[1]->GetWindowName(), SettingsId);
	ImGui::DockBuilderDockWindow(Panels[2]->GetWindowName(), LeftId);
	ImGui::DockBuilderDockWindow(Panels[3]->GetWindowName(), EmittersId);
	ImGui::DockBuilderFinish(InDockSpaceId);
}

void FParticleEditorTab::OnRender(bool bActive)
{
	ParticleEditor.BeginUIFrame();
	if (ParticleEditor.IsClosing())
	{
		return;
	}

	ParticleEditor.ReportPanelFocus(bActive && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));

	if (bActive)
	{
		DrawToolbar();
	}

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		const ImVec2 Size = ImGui::GetContentRegionAvail();
		if (bActive && Size.x > 0.0f && Size.y > 0.0f &&
			!ImGui::DockBuilderGetNode(DockSpaceId))
		{
			BuildDefaultLayout(DockSpaceId, Size);
		}

		ImGuiWindowClass WindowClass;
		WindowClass.ClassId = PanelClassId;
		WindowClass.DockingAllowUnclassed = false;
		const ImGuiDockNodeFlags Flags = bActive ? ImGuiDockNodeFlags_None : ImGuiDockNodeFlags_KeepAliveOnly;
		if (bActive || ImGui::DockBuilderGetNode(DockSpaceId))
		{
			ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f), Flags, &WindowClass);
		}
	}

	for (auto& Panel : Panels)
	{
		// 분리된 패널은 부모 탭이 가려져도 독립적으로 표시합니다.
		if (Panel->IsOpen() && (bActive || Panel->IsDetached(DockSpaceId)))
		{
			Panel->OnRender();
		}
	}

	if (ParticleEditor.IsFocused() && !ImGui::GetIO().WantTextInput && !ImGui::IsAnyItemActive() &&
		!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup) &&
		ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
	{
		ParticleEditor.Save();
	}

}
