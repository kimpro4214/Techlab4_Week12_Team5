#pragma once

#include "Editor/EditorUI/EditorTabContainer.h"
#include "Editor/LevelEditor/LevelEditorTab.h"

// 매니저 도입 전까지 레벨 탭을 소유하고 최상위 탭 UI를 연결한다.
class FEditorUI
{
public:
	~FEditorUI();
	bool Init();
	void OnRender();
	FLevelEditorTab* GetLevelEditorTab() const { return LevelEditorTab.get(); }

private:
	FEditorTabContainer TabContainer;
	TUniquePtr<FLevelEditorTab> LevelEditorTab;
};
