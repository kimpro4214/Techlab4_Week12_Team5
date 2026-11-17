#include "EnginePCH.h"
#include "ParticleEditorPanel.h"
#include "Editor/ParticleEditor/ParticleEditor.h"

FParticleEditorPanel::FParticleEditorPanel(FParticleEditor& InEditor, const char* InName)
	: Editor(InEditor)
	, PanelName(InName)
	, WindowName(PanelName + "###ParticlePanel:" + InName + ":" + InEditor.GetKey().GetTabId())
{
}

bool FParticleEditorPanel::Init()
{
	return true;
}

void FParticleEditorPanel::Tick(float DeltaTime)
{
}

void FParticleEditorPanel::OnRender()
{
	// 같은 에디터의 패널끼리 도킹하고, 다른 탭의 도킹 영역과 섞이지 않도록 합니다.
	ImGuiWindowClass WindowClass;
	WindowClass.ClassId = DockClassId;
	WindowClass.DockingAllowUnclassed = false;
	ImGui::SetNextWindowClass(&WindowClass);
	ImGui::SetNextWindowSize(ImVec2(480.0f, 320.0f), ImGuiCond_FirstUseEver);

	bool bOpen = IsOpen();
	if (ImGui::Begin(GetWindowName(), &bOpen))
	{
		Editor.ReportPanelFocus(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
		DrawContent();
	}
	ImGui::End();
	SetOpen(bOpen);
}

bool FParticleEditorPanel::IsDetached(ImGuiID DockSpaceId) const
{
	// 플랫폼 창 ID 대신 실제 도킹 트리의 루트를 기준으로 분리 여부를 판단합니다.
	const ImGuiWindow* Window = ImGui::FindWindowByName(GetWindowName());
	return Window && (!Window->DockNode || ImGui::DockNodeGetRootNode(Window->DockNode)->ID != DockSpaceId);
}
