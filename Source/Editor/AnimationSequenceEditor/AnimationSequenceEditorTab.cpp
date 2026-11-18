#include "EnginePCH.h"
#include "Editor/AnimationSequenceEditor/AnimationSequenceEditorTab.h"
#include "Editor/AnimationSequenceEditor/AnimationSequenceEditor.h"

FAnimationSequenceEditorTab::FAnimationSequenceEditorTab(FAnimationSequenceEditor& InEditor)
	: FAssetEditorTab(InEditor), SequenceEditor(InEditor)
{
	const ImGuiID AssetId = ImHashStr(InEditor.GetKey().GetTabId().c_str());
	DockSpaceId = ImHashStr("AnimationSequenceDockSpace", 0, AssetId);
	PanelClassId = ImHashStr("AnimationSequencePanelClass", 0, AssetId);
	Panels.Add(MakeUnique<FAnimationSequenceEditorPanel>(InEditor, FAnimationSequenceEditorPanel::ERole::Viewport, "Animation Viewport"));
	Panels.Add(MakeUnique<FAnimationSequenceEditorPanel>(InEditor, FAnimationSequenceEditorPanel::ERole::SequenceInfo, "Sequence Info"));
	Panels.Add(MakeUnique<FAnimationSequenceEditorPanel>(InEditor, FAnimationSequenceEditorPanel::ERole::Hierarchy, "Animation Bones"));
	Panels.Add(MakeUnique<FAnimationSequenceEditorPanel>(InEditor, FAnimationSequenceEditorPanel::ERole::NotifyDetails, "Notify Details"));
	Panels.Add(MakeUnique<FAnimationSequenceEditorPanel>(InEditor, FAnimationSequenceEditorPanel::ERole::Timeline, "Animation Timeline"));
	for (auto& Panel : Panels)
	{
		Panel->SetDockClass(PanelClassId);
		Panel->Init();
	}
}

void FAnimationSequenceEditorTab::DrawMenuBar()
{
	FAssetEditorTab::DrawMenuBar();
	if (ImGui::BeginMenu("Window"))
	{
		for (auto& Panel : Panels)
		{
			if (ImGui::MenuItem(Panel->GetPanelName(), nullptr, Panel->IsOpen())) Panel->SetOpen(!Panel->IsOpen());
		}
		ImGui::EndMenu();
	}
}

void FAnimationSequenceEditorTab::BuildDefaultLayout(const ImVec2& Size)
{
	ImGui::DockBuilderAddNode(DockSpaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(DockSpaceId, Size);
	ImGuiID ViewportId = DockSpaceId;
	ImGuiID RightId = ImGui::DockBuilderSplitNode(ViewportId, ImGuiDir_Right, 0.28f, nullptr, &ViewportId);
	const ImGuiID InfoId = ImGui::DockBuilderSplitNode(RightId, ImGuiDir_Up, 0.18f, nullptr, &RightId);
	const ImGuiID HierarchyId = ImGui::DockBuilderSplitNode(RightId, ImGuiDir_Up, 0.48f, nullptr, &RightId);
	const ImGuiID SettingsId = ImGui::DockBuilderSplitNode(ViewportId, ImGuiDir_Down, 0.32f, nullptr, &ViewportId);
	ImGui::DockBuilderDockWindow(Panels[0]->GetWindowName(), ViewportId);
	ImGui::DockBuilderDockWindow(Panels[1]->GetWindowName(), InfoId);
	ImGui::DockBuilderDockWindow(Panels[2]->GetWindowName(), HierarchyId);
	ImGui::DockBuilderDockWindow(Panels[3]->GetWindowName(), RightId);
	ImGui::DockBuilderDockWindow(Panels[4]->GetWindowName(), SettingsId);
	ImGui::DockBuilderFinish(DockSpaceId);
}

void FAnimationSequenceEditorTab::OnRender(bool bActive)
{
	SequenceEditor.BeginUIFrame();
	if (SequenceEditor.IsClosing()) return;
	SequenceEditor.ReportPanelFocus(bActive && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));

	const ImVec2 Size = ImGui::GetContentRegionAvail();
	if (bActive && Size.x > 0.0f && Size.y > 0.0f && !ImGui::DockBuilderGetNode(DockSpaceId))
		BuildDefaultLayout(Size);

	ImGuiWindowClass WindowClass;
	WindowClass.ClassId = PanelClassId;
	WindowClass.DockingAllowUnclassed = false;
	if (bActive || ImGui::DockBuilderGetNode(DockSpaceId))
	{
		const ImGuiDockNodeFlags Flags = bActive ? ImGuiDockNodeFlags_None : ImGuiDockNodeFlags_KeepAliveOnly;
		ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f), Flags, &WindowClass);
	}
	for (auto& Panel : Panels)
	{
		if (Panel->IsOpen() && (bActive || Panel->IsDetached(DockSpaceId))) Panel->OnRender();
	}
	HandleShortcuts();

}

void FAnimationSequenceEditorTab::HandleShortcuts()
{
	if (!SequenceEditor.IsFocused() || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive() ||
		ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) return;
	if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) SequenceEditor.TogglePlayback();
}
