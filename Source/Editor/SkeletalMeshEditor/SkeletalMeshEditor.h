#pragma once

#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/AssetEditor/FPreviewScene.h"
#include "Editor/Gizmo/BoneGizmoTarget.h"
#include "Editor/Gizmo/Gizmo.h"
#include "Editor/SkeletalMeshEditor/SkeletalPose.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Camera/ViewportCameraMovement.h"

#include <functional>

class UEngine;
class USkeletalMesh;
class USkeletalMeshComponent;
struct FEditorSettings;

struct FSkeletalMeshViewportInput
{
	FViewportCameraInput Camera;
	FVector2 LocalMouse;
	bool bHovered = false;
	bool bLeftPressed = false;
	bool bLeftDown = false;
	bool bItemActive = false;
};

class FSkeletalMeshEditor : public FAssetEditor
{
public:
	using FPreviewRenderer = std::function<void(FSkeletalMeshEditor&)>;

	FSkeletalMeshEditor(UEngine* InEngine, FEditorManager& InManager, const FAssetEditorKey& InKey,
		USkeletalMesh* InMesh, const FEditorSettings* InSettings, FPreviewRenderer InRenderer = {});

	void Tick(float DeltaTime) override;
	void RenderPreview() override;
	// 전용 탭이 패널 렌더링을 담당한다.
	void OnRender(bool) override {}
	bool IsDirty() const override;
	bool CanSave() const override { return Mesh != nullptr; }
	bool Save() override { return SavePoseToFile(); }
	bool IsFocused() const override { return bFocused; }

	void BeginUIFrame();
	void EndUIFrame(bool bAnyItemActive);
	void ReportPanelFocus(bool bInFocused) { bFocused |= bInFocused; }
	void SetViewportVisible(bool bVisible) { bViewportVisible = bVisible; }
	void ProcessViewportInput(const FSkeletalMeshViewportInput& Input, float DeltaTime);
	void SelectBone(int32 BoneIndex);
	void SetBoneLocalTransform(int32 BoneIndex, const FTransform& Transform);
	void ResetPose();
	void CommitUndo();
	void Undo();
	void Redo();
	bool ApplyPoseToAsset();
	bool SavePoseToFile();

	UWorld* GetPreviewWorld() const { return PreviewScene.GetWorld(); }
	FEditorViewportClient* GetViewportClient() { return &ViewportClient; }
	USkeletalMesh* GetMesh() const { return Mesh; }
	const FReferenceSkeleton& GetReferenceSkeleton() const { return ReferenceSkeleton; }
	const FSkeletalPose& GetPreviewPose() const { return PreviewPose; }
	const TArray<TArray<int32>>& GetBoneChildren() const { return Children; }
	int32 GetSelectedBoneIndex() const { return SelectedBoneIndex; }
	FGizmo& GetBoneGizmo() { return BoneGizmo; }
	const FGizmo& GetBoneGizmo() const { return BoneGizmo; }
	bool IsRenderGrid() const { return bRenderGrid; }
	bool IsRenderMesh() const { return bRenderMesh; }
	bool IsRenderBones() const { return bRenderBones || SelectedBoneIndex != -1; }
	bool IsBonesVisible() const { return bRenderBones; }
	bool IsBoneWeightRequested() const { return bShowBoneWeight; }
	bool IsBoneWeightEnabled() const;
	int32 GetBoneWeightBoneIndex() const;
	void SetRenderGrid(bool bValue) { bRenderGrid = bValue; }
	void SetRenderMesh(bool bValue);
	void SetRenderBones(bool bValue) { bRenderBones = bValue; }
	void SetBoneWeightEnabled(bool bValue) { bShowBoneWeight = bValue; }

protected:
	TUniquePtr<IEditorTab> CreateTab() override;
	bool InitializeResources() override;
	void ShutdownResources() override;
	bool DiscardChanges() override;

private:
	void SetSkeleton(const FReferenceSkeleton& Skeleton);
	void SyncPoseToComponent();
	int32 PickBone(const FVector2& MousePosition);

	UEngine* Engine = nullptr;
	USkeletalMesh* Mesh = nullptr;
	const FEditorSettings* Settings = nullptr;
	FPreviewRenderer PreviewRenderer;
	FPreviewScene PreviewScene;
	FEditorViewportClient ViewportClient;
	USkeletalMeshComponent* PreviewMeshComponent = nullptr;
	FReferenceSkeleton ReferenceSkeleton;
	FReferenceSkeleton SavedSkeleton;
	FSkeletalPose PreviewPose;
	TArray<TArray<int32>> Children;
	int32 SelectedBoneIndex = -1;
	FGizmo BoneGizmo;
	FBoneGizmoTarget BoneGizmoTarget;
	float CameraSpeed = 1.0f;
	bool bJustPicked = false;
	bool bViewportVisible = false;
	bool bFocused = false;
	bool bRenderGrid = false;
	bool bRenderMesh = true;
	bool bRenderBones = true;
	bool bShowBoneWeight = false;
	bool bAppliedToAsset = false;

	struct FBoneChange
	{
		int32 BoneIndex;
		FTransform Before;
		FTransform After;
	};
	TArray<TArray<FBoneChange>> UndoStack;
	TArray<TArray<FBoneChange>> RedoStack;
	TArray<FTransform> LastSnapshot;
};
