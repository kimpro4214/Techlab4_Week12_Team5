#pragma once

#include "Core/EngineString.h"
#include "Editor/EditorUI/EditorTab.h"

class FAssetEditor;

class FAssetEditorTab : public IEditorTab
{
public:
	explicit FAssetEditorTab(FAssetEditor& InEditor);

	const char* GetTabName() const override { return TabName.c_str(); }
	bool CanClose() const override { return true; }
	void DrawMenuBar() override;
	void OnRender(bool bActive) override;
	void OnCloseRequested() override;

private:
	FAssetEditor& Editor;
	FString TabName;
};
