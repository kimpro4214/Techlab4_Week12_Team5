#pragma once

#include "Editor/AssetEditor/AssetEditor.h"
#include "Render/SceneViewTargets.h"
#include "UI/UIDrawList.h"

class UClass;
class UWidget;
class UPanelWidget;
class FUIRenderer;
class FUIEditorPanel;
class FUIEditorTab;

class FUIEditor : public FAssetEditor
{
public:
	FUIEditor(FEditorManager& InManager, const FAssetEditorKey& InKey, FUIRenderer* InRenderer);
	~FUIEditor() override;

	static bool CreateAssetFile(const FString& Path);
	void OnRender(bool bActive) override {}
	void RenderPreview() override;
	bool IsFocused() const override { return bFocused; }
	bool IsDirty() const override;
	bool CanSave() const override { return Root != nullptr; }
	bool Save() override;

	void Undo();
	void Redo();
	void CommitUndoIfChanged(bool bEditing = false);
	void DeleteSelected();
	UWidget* AddWidget(UClass* Class);
	bool ReparentWidget(UWidget* Widget, UPanelWidget* NewParent);
	void SelectWidget(UWidget* Widget);
	void SetParentClassName(const FString& Name) { ParentClassName = Name; }
	UPanelWidget* GetRoot() const { return Root; }
	UWidget* GetSelected() const { return Selected; }
	const FString& GetParentClassName() const { return ParentClassName; }
	FVector2 GetPreviewSize() const { return PreviewSize; }
	bool IsCanvasVisible() const { return bCanvasVisible; }
	void BeginUIFrame();
	void ReportPanelFocus(bool bInFocused) { bFocused |= bInFocused; }
	void SetCanvasVisible() { bCanvasVisible = true; }
	void EndUIFrame(bool bEditing);

protected:
	TUniquePtr<IEditorTab> CreateTab() override;
	bool InitializeResources() override;
	void ShutdownResources() override;
	bool DiscardChanges() override;

private:
	friend class FUIEditorPanel;
	friend class FUIEditorTab;

	enum class EDragMode { None, Move, Resize };
	bool ContainsWidget(const UWidget* Widget) const;
	UWidget* PickWidget(UWidget* Widget, float X, float Y) const;
	void ApplyDrag(float DX, float DY);
	void SetAnchorsKeepRect(UWidget* Widget, const FVector4& NewAnchors);
	FString Snapshot() const;
	bool RestoreSnapshot(const FString& Data, bool bRestorePreviewSize = false);

	FUIRenderer* Renderer = nullptr;
	FString ParentClassName = "UUserWidget";
	UPanelWidget* Root = nullptr;
	UWidget* Selected = nullptr;
	FVector2 PreviewSize = FVector2(1920.0f, 1080.0f);
	FSceneViewTargets PreviewTargets;
	FUIDrawList DrawList;
	EDragMode DragMode = EDragMode::None;
	float DragStartX = 0.0f;
	float DragStartY = 0.0f;
	FVector4 DragStartOffsets;
	struct FReparent { UWidget* Widget = nullptr; UPanelWidget* NewParent = nullptr; } PendingReparent;
	TArray<FString> UndoStack;
	TArray<FString> RedoStack;
	FString LastSnapshot;
	FString SavedSnapshot;
	bool bFocused = false;
	bool bCanvasVisible = false;
};
