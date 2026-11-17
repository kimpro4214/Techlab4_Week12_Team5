#pragma once

#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Editor/UIEditor/UIEditorPanel.h"

class FUIEditor;

class FUIEditorTab : public FAssetEditorTab
{
public:
	explicit FUIEditorTab(FUIEditor& InEditor);
	void DrawMenuBar() override;
	void OnRender(bool bActive) override;

private:
	void DrawToolbar();
	void HandleShortcuts();
	void BuildDefaultLayout(const ImVec2& Size);

	FUIEditor& UIEditor;
	TArray<TUniquePtr<FUIEditorPanel>> Panels;
	ImGuiID DockSpaceId = 0;
	ImGuiID PanelClassId = 0;
};
