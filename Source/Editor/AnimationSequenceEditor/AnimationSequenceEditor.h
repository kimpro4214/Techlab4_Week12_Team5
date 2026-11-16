#pragma once

#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/AssetEditor/FPreviewScene.h"
#include "Editor/EditorUI/EditorTimeline.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Engine/Classes/Animation/AnimSequence.h"
#include "Camera/ViewportCameraMovement.h"
#include <functional>

class UEngine;
class USkeletalMesh;
class USkeletalMeshComponent;
struct FEditorSettings;

class FAnimationSequenceEditor : public FAssetEditor
{
public:
	using FPreviewRenderer = std::function<void(FAnimationSequenceEditor&)>;
	FAnimationSequenceEditor(UEngine* InEngine, FEditorManager& InManager, const FAssetEditorKey& InKey,
		UAnimSequence* InSequence, USkeletalMesh* InMesh, const FEditorSettings* InSettings, FPreviewRenderer InRenderer = {});
	void Tick(float DeltaTime) override;
	void RenderPreview() override;
	void OnRender(bool) override {}
	bool IsFocused() const override { return bFocused; }
	void BeginUIFrame() { bFocused = false; bVisible = false; bViewportVisible = false; }
	void SetPanelVisible() { bVisible = true; }
	void SetViewportVisible() { bViewportVisible = true; bVisible = true; }
	void SetLooping(bool bValue) { bLooping = bValue; }
	void SetPlayRate(float Value);
	void SelectBone(int32 Index);
	int32 GetSelectedBoneIndex() const { return SelectedBoneIndex; }
	int32 GetSelectedNotifyIndex() const { return SelectedNotifyIndex; }
	void RemoveNotify(int32 Index);
	void UpdateNotify(int32 Index, const char* Name, float Time, float Duration);
	void ProcessCameraInput(const FViewportCameraInput& Input, float DeltaTime);
	void ReportPanelFocus(bool bValue) { bFocused |= bValue; }
	UWorld* GetPreviewWorld() const { return PreviewScene.GetWorld(); }
	FEditorViewportClient* GetViewportClient() { return &ViewportClient; }
	bool IsRenderGrid() const { return bRenderGrid; }
	bool IsBoneWeightEnabled() const;
	int32 GetBoneWeightBoneIndex() const;
	void TogglePlayback();
	void StepFrame(int32 Direction);
	void SetPreviewTime(float NewTime);
	float GetCurrentTime() const { return CurrentTime; }
	bool IsPlaying() const { return bPlaying; }
	int32 AddNotifyEvent(const char* InName, float InTriggerTime, float InDuration);
	void SelectNotify(int32 Index);

protected:
	TUniquePtr<IEditorTab> CreateTab() override;
	bool InitializeResources() override;
	void ShutdownResources() override;

private:
	friend class FAnimationSequenceEditorPanel;
	friend class FAnimationSequenceEditorTab;
	bool OpenAsset(UAnimSequence* InSequence, USkeletalMesh* InSkeletalMesh);
	void EvaluatePreviewPose();
	enum class ENotifyDragMode : uint8 { None, Move, ResizeEnd };
	UEngine* Engine;
	UAnimSequence* InitialSequence;
	USkeletalMesh* InitialMesh;
	FPreviewRenderer PreviewRenderer;
	bool bFocused = false;
	bool bVisible = false;
	bool bViewportVisible = false;
	bool bRenderGrid = true;
	bool bShowBoneWeight = false;
	// 애니메이션의 트랙 순서가 아닌, 프리뷰 메시의 스켈레톤 인덱스를 사용한다.
	int32 SelectedBoneIndex = -1;
	TArray<TArray<int32>> BoneChildren;
	float CameraSpeed = 1.0f;

	FPreviewScene PreviewScene;
	USkeletalMeshComponent* PreviewMeshComponent = nullptr;
	const FEditorSettings* Settings = nullptr;

	// AssetManager가 관리하는 에셋을 참조
	UAnimSequence* EditSequence = nullptr;
	float CurrentTime = 0.0f;
	bool bPlaying = false;
	bool bLooping = true;
	float PlayRate = 1.0f;
	EditorTimeline::FView TimelineView;
	bool bFitTimeline = true;

	// AnimNotify 편집 상태
	int32 SelectedNotifyIndex = INDEX_NONE;
	int32 DraggingNotifyIndex = INDEX_NONE;
	ENotifyDragMode DragMode = ENotifyDragMode::None;
	float PendingNotifyTime = 0.0f;
	float NewNotifyDuration = 0.0f;
	char NewNotifyName[128] = "Footstep";
	char EditNotifyName[128] = "";

	FEditorViewportClient ViewportClient;
};
