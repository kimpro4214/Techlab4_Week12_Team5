#pragma once

#include "Core/Delegate/Delegate.h"
#include "Editor/EditorUI/EditorTab.h"
#include "Editor/LevelEditor/LevelEditorToolbar.h"

#include <functional>

class FLevelEditorTab : public IEditorTab
{
public:
	bool Init(bool bInUseDockSpace = true, bool bInPassthruCentralNode = false);
	void Tick(float DeltaTime);
	void OnRender(bool bActive) override;
	const char* GetTabName() const override { return "LevelEditor###LevelEditor"; }
	bool CanDetach() const override { return false; }
	void DrawMenuBar() override;

	template <typename T>
	T* AddEditorPanel()
	{
		TUniquePtr<T> Panel = MakeUnique<T>();
		T* Ret = Panel.get();
		Ret->Init();
		Panels.Add(std::move(Panel));

		return Ret;
	}

	FLevelEditorToolbar* GetToolbar() const { return Toolbar.get(); }

	TDelegate<> OnNewScene;
	TDelegate<> OnOpenScene;
	TDelegate<> OnSaveScene;
	TDelegate<> OnSaveSceneAs;
	TDelegate<> OnPackageGame;

	// 패키징 중이면 true. Build 메뉴 항목을 비활성화하는 데 쓴다.
	void SetIsPackagingQuery(std::function<bool()> InQuery) { IsPackaging = InQuery; }

private:
	bool bUseDockSpace = true;
	bool bPassthruCentralNode = false;

	TArray<TUniquePtr<IEditorPanel>> Panels;
	TUniquePtr<FLevelEditorToolbar> Toolbar;

	std::function<bool()> IsPackaging;
};
