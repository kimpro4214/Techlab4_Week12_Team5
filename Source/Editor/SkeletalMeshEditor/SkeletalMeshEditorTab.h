#pragma once

#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshEditorPanel.h"

class FSkeletalMeshEditor;

class FSkeletalMeshEditorTab : public FAssetEditorTab
{
public:
	explicit FSkeletalMeshEditorTab(FSkeletalMeshEditor& InEditor);
	void DrawMenuBar() override;
	void OnRender(bool bActive) override;

private:
	void BuildDefaultLayout(const ImVec2& Size);
	void HandleShortcuts();

	FSkeletalMeshEditor& MeshEditor;
	TArray<TUniquePtr<FSkeletalMeshEditorPanel>> Panels;
	ImGuiID DockSpaceId = 0;
	ImGuiID PanelClassId = 0;
};
