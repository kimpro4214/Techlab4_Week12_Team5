#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/EditorUI/EditorTabContainer.h"
#include "Editor/LevelEditor/LevelEditorTab.h"

#include <functional>

class FEditorManager
{
public:
	using FEditorFactory = std::function<TUniquePtr<FAssetEditor>(FEditorManager&, const FAssetEditorKey&)>;

	~FEditorManager();
	bool Init();
	void RegisterFactory(const FString& EditorType, FEditorFactory Factory);
	bool OpenEditor(const FAssetEditorKey& Key);
	FAssetEditor* FindEditor(const FAssetEditorKey& Key) const;

	void Tick(float DeltaTime);
	void RenderPreview();
	void DrawEditorUI();
	// ImGui 분리 창의 렌더링과 해당 프레임의 참조가 끝난 뒤 호출한다.
	void LateTick();
	void Shutdown();

	FLevelEditorTab* GetLevelEditorTab() const { return LevelEditorTab.get(); }
	int32 GetEditorCount() const { return Editors.Num(); }
	IEditorTab* GetMainSelectedTab() const { return TabContainer.GetMainSelectedTab(); }
	IEditorTab* GetFocusedTab() const { return TabContainer.GetFocusedTab(); }
	bool HasFocusedAssetEditor() const;
	bool IsLevelEditorInputEnabled() const;

	void AddEditorTab(IEditorTab* Tab) { TabContainer.AddTab(Tab); }
	void RemoveEditorTab(IEditorTab* Tab) { TabContainer.RemoveTab(Tab); }
	void ActivateEditorTab(IEditorTab* Tab) { TabContainer.ActivateTab(Tab); }
	void RequestCloseEditorTab(IEditorTab* Tab) { TabContainer.RequestCloseTab(Tab); }

private:
	static bool NormalizeKey(const FAssetEditorKey& InKey, FAssetEditorKey& OutKey);
	void CreateEditor(const FAssetEditorKey& Key);
	void DrawCloseConfirmation();

	FEditorTabContainer TabContainer;
	TUniquePtr<FLevelEditorTab> LevelEditorTab;
	TArray<TUniquePtr<FAssetEditor>> Editors;
	TMap<FString, FEditorFactory> Factories;
	TArray<FAssetEditorKey> PendingOpens;
	FAssetEditor* ClosePromptEditor = nullptr;
	bool bCloseActionFailed = false;
	bool bShuttingDown = false;
	bool bInitialized = false;
};
