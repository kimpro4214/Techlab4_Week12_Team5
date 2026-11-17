#include "EnginePCH.h"
#include "ParticleEditorTab.h"
#include "ParticleEditor.h"
#include "Panels/ParticleViewportPanel.h"
#include "Panels/ParticleDetailsPanel.h"
#include "Panels/ParticleEmittersPanel.h"

FParticleEditorTab::FParticleEditorTab(FParticleEditor& InEditor)
	: FAssetEditorTab(InEditor)
	, ParticleEditor(InEditor)
{
	// 탭의 부모 창이 달라져도 애셋별 도킹 ID를 유지합니다.
	const ImGuiID AssetId = ImHashStr(InEditor.GetKey().GetTabId().c_str());
	DockSpaceId = ImHashStr("ParticleDockSpaceV2", 0, AssetId);
	PanelClassId = ImHashStr("ParticlePanelClass", 0, AssetId);

	Panels.Add(MakeUnique<FParticleViewportPanel>(InEditor));
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

void FParticleEditorTab::BuildDefaultLayout(ImGuiID InDockSpaceId, const ImVec2& Size)
{
	ImGui::DockBuilderAddNode(InDockSpaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(InDockSpaceId, Size);
	ImGuiID EmittersId = InDockSpaceId;
	ImGuiID LeftId = ImGui::DockBuilderSplitNode(EmittersId, ImGuiDir_Left, 0.25f, nullptr, &EmittersId);
	ImGuiID ViewportId = LeftId;
	const ImGuiID DetailsId = ImGui::DockBuilderSplitNode(ViewportId, ImGuiDir_Down, 0.5f, nullptr, &ViewportId);

	ImGui::DockBuilderDockWindow(Panels[0]->GetWindowName(), ViewportId);
	ImGui::DockBuilderDockWindow(Panels[1]->GetWindowName(), DetailsId);
	ImGui::DockBuilderDockWindow(Panels[2]->GetWindowName(), EmittersId);
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
