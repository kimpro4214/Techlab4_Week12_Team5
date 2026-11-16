#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditorTab.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshViewportPanel.h"
#include "Editor/SkeletalMeshEditor/Panels/BoneHierarchyPanel.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshDetailsPanel.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshInfoPanel.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshSettingsPanel.h"

FSkeletalMeshEditorTab::FSkeletalMeshEditorTab(FSkeletalMeshEditor& InEditor)
	: FAssetEditorTab(InEditor), MeshEditor(InEditor)
{
	const ImGuiID AssetId = ImHashStr(InEditor.GetKey().GetTabId().c_str());
	DockSpaceId = ImHashStr("SkeletalMeshDockSpace", 0, AssetId);
	PanelClassId = ImHashStr("SkeletalMeshPanelClass", 0, AssetId);
	Panels.Add(MakeUnique<FSkeletalMeshViewportPanel>(InEditor));
	Panels.Add(MakeUnique<FSkeletalMeshInfoPanel>(InEditor));
	Panels.Add(MakeUnique<FBoneHierarchyPanel>(InEditor));
	Panels.Add(MakeUnique<FSkeletalMeshDetailsPanel>(InEditor));
	Panels.Add(MakeUnique<FSkeletalMeshSettingsPanel>(InEditor));
	for (auto& Panel : Panels)
	{
		Panel->SetDockClass(PanelClassId);
		Panel->Init();
	}
}

void FSkeletalMeshEditorTab::DrawMenuBar()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Apply To Asset", "Ctrl+S")) MeshEditor.ApplyPoseToAsset();
		if (ImGui::MenuItem("Save Pose File...", "Ctrl+Shift+S")) MeshEditor.SavePoseToFile();
		if (ImGui::MenuItem("Close Editor")) MeshEditor.Close();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "Ctrl+Z")) MeshEditor.Undo();
		if (ImGui::MenuItem("Redo", "Ctrl+Y")) MeshEditor.Redo();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Window"))
	{
		for (auto& Panel : Panels)
		{
			if (ImGui::MenuItem(Panel->GetPanelName(), nullptr, Panel->IsOpen())) Panel->SetOpen(!Panel->IsOpen());
		}
		ImGui::EndMenu();
	}
}

void FSkeletalMeshEditorTab::BuildDefaultLayout(const ImVec2& Size)
{
	ImGui::DockBuilderAddNode(DockSpaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(DockSpaceId, Size);
	ImGuiID ViewportId = DockSpaceId;
	ImGuiID RightId = ImGui::DockBuilderSplitNode(ViewportId, ImGuiDir_Right, 0.28f, nullptr, &ViewportId);
	const ImGuiID InfoId = ImGui::DockBuilderSplitNode(RightId, ImGuiDir_Up, 0.18f, nullptr, &RightId);
	const ImGuiID HierarchyId = ImGui::DockBuilderSplitNode(RightId, ImGuiDir_Up, 0.48f, nullptr, &RightId);
	const ImGuiID SettingsId = ImGui::DockBuilderSplitNode(RightId, ImGuiDir_Down, 0.35f, nullptr, &RightId);
	ImGui::DockBuilderDockWindow(Panels[0]->GetWindowName(), ViewportId);
	ImGui::DockBuilderDockWindow(Panels[1]->GetWindowName(), InfoId);
	ImGui::DockBuilderDockWindow(Panels[2]->GetWindowName(), HierarchyId);
	ImGui::DockBuilderDockWindow(Panels[3]->GetWindowName(), RightId);
	ImGui::DockBuilderDockWindow(Panels[4]->GetWindowName(), SettingsId);
	ImGui::DockBuilderFinish(DockSpaceId);
}

void FSkeletalMeshEditorTab::OnRender(bool bActive)
{
	MeshEditor.BeginUIFrame();
	if (MeshEditor.IsClosing()) return;
	MeshEditor.ReportPanelFocus(bActive && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));

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
	MeshEditor.EndUIFrame(ImGui::IsAnyItemActive());
}

void FSkeletalMeshEditorTab::HandleShortcuts()
{
	const ImGuiIO& IO = ImGui::GetIO();
	if (!MeshEditor.IsFocused() || IO.WantTextInput || ImGui::IsAnyItemActive() ||
		MeshEditor.GetBoneGizmo().IsUsing() || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) return;
	if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
	{
		const int NextMode = (static_cast<int>(MeshEditor.GetBoneGizmo().GetMode()) + 1) % 3;
		MeshEditor.GetBoneGizmo().SetMode(static_cast<EGizmoMode>(NextMode));
	}
	if (IO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) IO.KeyShift ? MeshEditor.Redo() : MeshEditor.Undo();
	else if (IO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) MeshEditor.Redo();
	else if (IO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
	{
		if (IO.KeyShift) MeshEditor.SavePoseToFile();
		else MeshEditor.ApplyPoseToAsset();
	}
}
