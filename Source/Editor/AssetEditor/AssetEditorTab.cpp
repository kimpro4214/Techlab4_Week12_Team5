#include "EnginePCH.h"
#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Editor/AssetEditor/AssetEditor.h"

FAssetEditorTab::FAssetEditorTab(FAssetEditor& InEditor)
	: Editor(InEditor)
{
	const FAssetEditorKey& Key = Editor.GetKey();
	const FString Name = Key.SubAssetName.empty()
		? std::filesystem::path(Key.AssetPath).filename().string() : Key.SubAssetName;
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
