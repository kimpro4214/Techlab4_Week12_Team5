#pragma once

#include "Core/EngineString.h"
#include "Core/Types.h"

class FEditorManager;
class IEditorTab;

// 파일 하나에 여러 클립이 있어도 각각 독립된 편집기를 열 수 있다.
struct FAssetEditorKey
{
	FString EditorType;
	FString AssetPath;
	FString SubAssetName;

	bool operator==(const FAssetEditorKey& Other) const;
	FString GetTabId() const;
};

enum class EAssetEditorCloseState
{
	None,
	AwaitingDecision,
	Confirmed
};

class FAssetEditor
{
public:
	FAssetEditor(FEditorManager& InManager, const FAssetEditorKey& InKey);
	virtual ~FAssetEditor();

	bool Initialize();
	void Shutdown();
	void Activate();
	void Close();
	bool SaveAndClose();
	bool DiscardAndClose();
	void CancelClose();

	virtual void Tick(float DeltaTime) {}
	virtual void RenderPreview() {}
	virtual void OnRender(bool bActive) = 0;
	virtual void DrawMenuBar() {}
	virtual bool IsFocused() const { return false; }
	virtual bool IsDirty() const { return false; }
	virtual bool CanSave() const { return false; }
	virtual bool Save() { return false; }

	const FAssetEditorKey& GetKey() const { return Key; }
	EAssetEditorCloseState GetCloseState() const { return CloseState; }
	bool IsClosing() const { return CloseState == EAssetEditorCloseState::Confirmed; }

protected:
	virtual TUniquePtr<IEditorTab> CreateTab();
	virtual bool InitializeResources() { return true; }
	virtual void ShutdownResources() {}
	// 공유 원본을 수정하는 편집기는 실제 복원에 성공했을 때만 true를 반환한다.
	virtual bool DiscardChanges() { return !IsDirty(); }

	FEditorManager& Manager;

private:
	friend class FEditorManager;

	FAssetEditorKey Key;
	TUniquePtr<IEditorTab> Tab;
	EAssetEditorCloseState CloseState = EAssetEditorCloseState::None;
	bool bInitialized = false;
	bool bShutdown = false;
};
