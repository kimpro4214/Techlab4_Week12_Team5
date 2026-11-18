#pragma once

#include "Editor/AssetEditor/AssetEditorTab.h"
#include "Panels/ParticleEditorPanel.h"

class FParticleEditor;

class FParticleEditorTab : public FAssetEditorTab
{
public:
	explicit FParticleEditorTab(FParticleEditor& InEditor);

	// 탭의 메뉴 바를 그립니다.
	void DrawMenuBar() override;

	// 탭의 렌더링을 수행합니다.
	void OnRender(bool bActive) override;

private:
	void DrawToolbar();
	void BuildDefaultLayout(ImGuiID InDockSpaceId, const ImVec2& Size);

	FParticleEditor& ParticleEditor;
	TArray<TUniquePtr<FParticleEditorPanel>> Panels;
	ImGuiID DockSpaceId = 0;
	ImGuiID PanelClassId = 0;
};
