#include "EnginePCH.h"
#include "Editor/LevelEditor/LevelEditorTab.h"

bool FLevelEditorTab::Init(bool bInUseDockSpace, bool bInPassthruCentralNode)
{
	bUseDockSpace = bInUseDockSpace;
	bPassthruCentralNode = bInPassthruCentralNode;

	Toolbar = MakeUnique<FLevelEditorToolbar>();

	return true;
}

void FLevelEditorTab::Tick(float DeltaTime)
{
	for (auto& Panel : Panels)
	{
		Panel->Tick(DeltaTime);
	}
}

void FLevelEditorTab::OnRender(bool bActive)
{
	if (bActive && Toolbar) Toolbar->Draw();

	if (bUseDockSpace && (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable))
	{
		// 기존 DockingSpace 창에서 사용하던 ID를 유지해 패널 배치를 보존한다.
		const ImGuiID DockSpaceId = ImHashStr("EngineDockingSpace", 0, ImHashStr("DockingSpace"));
		ImGuiDockNodeFlags Flags = bPassthruCentralNode
			? ImGuiDockNodeFlags_PassthruCentralNode : ImGuiDockNodeFlags_None;
		if (!bActive) Flags |= ImGuiDockNodeFlags_KeepAliveOnly;

		ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f), Flags);
	}

	if (bActive)
	{
		for (auto& Panel : Panels)
		{
			if (Panel->IsOpen()) Panel->OnRender();
		}
	}
}

// 파일 메뉴의 요청을 애플리케이션 콜백으로 전달한다.
void FLevelEditorTab::DrawMenuBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 10.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 10.0f));

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Scene")) { OnNewScene.Broadcast(); }
		if (ImGui::MenuItem("Open Scene...")) { OnOpenScene.Broadcast(); }

		ImGui::Separator();

		if (ImGui::MenuItem("Save")) { OnSaveScene.Broadcast(); }
		if (ImGui::MenuItem("Save As...")) { OnSaveSceneAs.Broadcast(); }

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		for (auto& Panel : Panels)
		{
			bool bOpen = Panel->IsOpen();

			if (ImGui::MenuItem(Panel->GetPanelName(), nullptr, bOpen))
			{
				Panel->SetOpen(!bOpen);
			}
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Build"))
	{
		const bool bCanPackage = !IsPackaging || !IsPackaging();
		if (ImGui::MenuItem("Package Game (Release)", nullptr, false, bCanPackage)) { OnPackageGame.Broadcast(); }
		ImGui::EndMenu();
	}

	ImGui::PopStyleVar(2);
}
