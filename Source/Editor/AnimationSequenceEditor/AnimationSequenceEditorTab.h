#pragma once

#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Editor/AnimationSequenceEditor/AnimationSequenceEditorPanel.h"

class FAnimationSequenceEditor;

class FAnimationSequenceEditorTab : public FAssetEditorTab
{
public:
	explicit FAnimationSequenceEditorTab(FAnimationSequenceEditor& InEditor);
	void DrawMenuBar() override;
	void OnRender(bool bActive) override;

private:
	void BuildDefaultLayout(const ImVec2& Size);
	void HandleShortcuts();

	FAnimationSequenceEditor& SequenceEditor;
	TArray<TUniquePtr<FAnimationSequenceEditorPanel>> Panels;
	ImGuiID DockSpaceId = 0;
	ImGuiID PanelClassId = 0;
};
