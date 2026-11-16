#include "EnginePCH.h"
#include "Editor/EditorUI/EditorUI.h"

FEditorUI::~FEditorUI()
{
	// 탭이 파괴되기 전에 컨테이너의 비소유 참조를 해제한다.
	TabContainer.RemoveTab(LevelEditorTab.get());
}

bool FEditorUI::Init()
{
	LevelEditorTab = MakeUnique<FLevelEditorTab>();
	if (!LevelEditorTab->Init()) return false;

	TabContainer.AddTab(LevelEditorTab.get());
	TabContainer.ActivateTab(LevelEditorTab.get());
	return true;
}

void FEditorUI::OnRender()
{
	TabContainer.OnRender();
}
