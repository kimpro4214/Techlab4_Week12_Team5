#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshEditorPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"

FSkeletalMeshEditorPanel::FSkeletalMeshEditorPanel(FSkeletalMeshEditor& InEditor, const char* InName)
	: Editor(InEditor), PanelName(InName)
{
	WindowName = PanelName + "###SkeletalMeshPanel:" + PanelName + ":" + Editor.GetKey().GetTabId();
}

void FSkeletalMeshEditorPanel::OnRender()
{
	ImGuiWindowClass WindowClass;
	WindowClass.ClassId = DockClassId;
	WindowClass.DockingAllowUnclassed = false;
	ImGui::SetNextWindowClass(&WindowClass);
	bool bOpen = IsOpen();
	if (ImGui::Begin(GetWindowName(), &bOpen))
	{
		Editor.ReportPanelFocus(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
		DrawContent();
	}
	SetOpen(bOpen);
	ImGui::End();
}

bool FSkeletalMeshEditorPanel::IsDetached(ImGuiID DockSpaceId) const
{
	const ImGuiWindow* Window = ImGui::FindWindowByName(GetWindowName());
	return Window && (!Window->DockNode || ImGui::DockNodeGetRootNode(Window->DockNode)->ID != DockSpaceId);
}
