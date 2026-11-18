#include "EnginePCH.h"
#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Editor/AssetEditor/AssetEditor.h"

FAssetEditorTab::FAssetEditorTab(FAssetEditor& InEditor)
	: Editor(InEditor)
{
	const FAssetEditorKey& Key = Editor.GetKey();
	// 에디터 키의 경로는 '/'로 정규화되어 있으므로 인코딩 변환 없이 파일명을 얻는다.
	const size_t Separator = Key.AssetPath.find_last_of('/');
	const FString Name = Key.SubAssetName.empty()
		? Key.AssetPath.substr(Separator == FString::npos ? 0 : Separator + 1) : Key.SubAssetName;
	TabName = Name + " (" + Key.EditorType + ")###AssetEditor:" + Key.GetTabId();
}

void FAssetEditorTab::DrawMenuBar()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Save", nullptr, false, Editor.CanSave())) Editor.Save();
		if (ImGui::MenuItem("Close Editor")) Editor.Close();
		ImGui::EndMenu();
	}
	Editor.DrawMenuBar();
}

void FAssetEditorTab::OnRender(bool bActive)
{
	if (!Editor.IsClosing()) Editor.OnRender(bActive);
}

void FAssetEditorTab::OnCloseRequested()
{
	Editor.Close();
}
