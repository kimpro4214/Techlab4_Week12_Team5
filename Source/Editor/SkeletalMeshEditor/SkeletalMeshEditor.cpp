#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditorTab.h"

#include "Asset/FbxImporter/SkeletalPoseFile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor/Settings/SettingsPanel.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Render/SkeletalMesh.h"

namespace
{
	bool PosesMatch(const TArray<FTransform>& A, const TArray<FTransform>& B)
	{
		if (A.Num() != B.Num()) return false;
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index] != B[Index]) return false;
		}
		return true;
	}
}

FSkeletalMeshEditor::FSkeletalMeshEditor(UEngine* InEngine, FEditorManager& InManager,
	const FAssetEditorKey& InKey, USkeletalMesh* InMesh, const FEditorSettings* InSettings, FPreviewRenderer InRenderer)
	: FAssetEditor(InManager, InKey), Engine(InEngine), Mesh(InMesh), Settings(InSettings),
	PreviewRenderer(std::move(InRenderer))
{
}

TUniquePtr<IEditorTab> FSkeletalMeshEditor::CreateTab()
{
	return MakeUnique<FSkeletalMeshEditorTab>(*this);
}

bool FSkeletalMeshEditor::InitializeResources()
{
	if (!Mesh || Mesh->GetRefSkeleton().RawRefBoneInfo.IsEmpty() ||
		Mesh->GetRefSkeleton().RawRefBoneInfo.Num() != Mesh->GetRefSkeleton().RawRefBonePose.Num()) return false;

	FPreviewSceneOptions Options;
	Options.WorldName = "SkeletalMeshPreview";
	Options.bUseEditorSkySphere = true;
	if (!PreviewScene.Initialize(Engine, Options)) return false;
	ViewportClient.SetWorld(PreviewScene.GetWorld());
	PreviewMeshComponent = Cast<USkeletalMeshComponent>(
		PreviewScene.AddComponent(USkeletalMeshComponent::StaticClass(), "PreviewMesh"));
	if (!PreviewMeshComponent) return false;
	PreviewMeshComponent->SetSkeletalMesh(Mesh);
	if (PreviewMeshComponent->GetSkeletalMesh() != Mesh) return false;

	SavedSkeleton = Mesh->GetRefSkeleton();
	SetSkeleton(SavedSkeleton);
	return true;
}

void FSkeletalMeshEditor::ShutdownResources()
{
	BoneGizmo.EndDrag();
	BoneGizmo.SetTarget(nullptr);
	BoneGizmoTarget.SetBone(nullptr, -1);
	ViewportClient.SetWorld(nullptr);
	PreviewMeshComponent = nullptr;
	bViewportVisible = false;
	bFocused = false;
	PreviewScene.Release();
	Mesh = nullptr;
}

void FSkeletalMeshEditor::Tick(float DeltaTime)
{
	PreviewPose.UpdateComponentSpace();
	SyncPoseToComponent();
	PreviewScene.Tick(DeltaTime);
}

void FSkeletalMeshEditor::RenderPreview()
{
	if (bViewportVisible && PreviewRenderer) PreviewRenderer(*this);
}

void FSkeletalMeshEditor::BeginUIFrame()
{
	bViewportVisible = false;
	bFocused = false;
}

void FSkeletalMeshEditor::EndUIFrame(bool bAnyItemActive)
{
	if (!bViewportVisible) BoneGizmo.EndDrag();
	if (!bAnyItemActive && !BoneGizmo.IsUsing()) CommitUndo();
}

void FSkeletalMeshEditor::SetRenderMesh(bool bValue)
{
	bRenderMesh = bValue;
	if (PreviewMeshComponent) PreviewMeshComponent->SetVisibility(bValue);
}

void FSkeletalMeshEditor::SetBoneLocalTransform(int32 BoneIndex, const FTransform& Transform)
{
	if (PreviewPose.LocalPose.IsValidIndex(BoneIndex)) PreviewPose.LocalPose[BoneIndex] = Transform;
}

void FSkeletalMeshEditor::ResetPose()
{
	PreviewPose.LocalPose = ReferenceSkeleton.RawRefBonePose;
	CommitUndo();
}

bool FSkeletalMeshEditor::IsDirty() const
{
	return !PosesMatch(PreviewPose.LocalPose, SavedSkeleton.RawRefBonePose) ||
		(bAppliedToAsset && Mesh && !PosesMatch(Mesh->GetRefSkeleton().RawRefBonePose, SavedSkeleton.RawRefBonePose));
}

bool FSkeletalMeshEditor::DiscardChanges()
{
	if (bAppliedToAsset && Mesh) Mesh->SetRefSkeleton(SavedSkeleton);
	bAppliedToAsset = false;
	SetSkeleton(SavedSkeleton);
	SyncPoseToComponent();
	return true;
}

bool FSkeletalMeshEditor::ApplyPoseToAsset()
{
	if (!Mesh) return false;
	CommitUndo();
	ReferenceSkeleton.RawRefBonePose = PreviewPose.LocalPose;
	Mesh->SetRefSkeleton(ReferenceSkeleton);
	bAppliedToAsset = true;
	return true;
}

bool FSkeletalMeshEditor::SavePoseToFile()
{
	if (!Mesh) return false;
	const FString PosePath = std::filesystem::path(GetKey().AssetPath).replace_extension(".pose").generic_string();
	FString Error;
	// 파일 저장 실패 시 공유 원본과 저장 기준은 변경하지 않는다.
	if (!SavePoseFile(ReferenceSkeleton, PreviewPose.LocalPose, PosePath, Error))
	{
		HTR_LOG(Error, "Failed to save pose file '{}': {}", PosePath, Error);
		return false;
	}
	ApplyPoseToAsset();
	SavedSkeleton = ReferenceSkeleton;
	bAppliedToAsset = false;
	return true;
}

void FSkeletalMeshEditor::ProcessViewportInput(const FSkeletalMeshViewportInput& Input, float DeltaTime)
{
	const float MoveSpeed = CameraSpeed * (Settings ? Settings->CameraSpeed : 1.0f);
	const float Sensitivity = 0.2f * (Settings ? Settings->MouseSensitivity : 1.0f);
	ViewportClient.ApplyCameraInput(Input.Camera, DeltaTime, MoveSpeed, Sensitivity);

	if (!Input.bLeftDown) bJustPicked = false;
	const bool bMouseDown = Input.bItemActive && Input.bLeftDown && !bJustPicked;
	BoneGizmo.Update(ViewportClient.DeprojectScreenToWorld(Input.LocalMouse), Input.LocalMouse,
		ViewportClient.GetViewProjectionMatrix(), static_cast<int32>(ViewportClient.GetWidth()),
		static_cast<int32>(ViewportClient.GetHeight()), bMouseDown, ViewportClient.GetViewLocation(),
		!ViewportClient.IsPerspective());

	if (Input.bHovered && Input.bLeftPressed && !BoneGizmo.IsUsing() && BoneGizmo.GetHoveredAxis() < 0)
	{
		const int32 PickedBone = PickBone(Input.LocalMouse);
		bJustPicked = PickedBone >= 0;
		SelectBone(PickedBone);
	}
}

bool FSkeletalMeshEditor::IsBoneWeightEnabled() const
{
	return bShowBoneWeight && USkeletalMeshComponent::IsGPUSkinningEnabled() &&
		PreviewMeshComponent && PreviewMeshComponent->GetSkeletalMesh();
}

int32 FSkeletalMeshEditor::GetBoneWeightBoneIndex() const
{
	if (!IsBoneWeightEnabled() ||
		SelectedBoneIndex < 0 || SelectedBoneIndex >= ReferenceSkeleton.RawRefBoneInfo.Num())
		return -1;
	return SelectedBoneIndex;
}

void FSkeletalMeshEditor::SetSkeleton(const FReferenceSkeleton& Skeleton)
{
	ReferenceSkeleton = Skeleton;
	PreviewPose.Skeleton = &ReferenceSkeleton;
	PreviewPose.Reset();

	Children.Reset();
	Children.SetNum(ReferenceSkeleton.RawRefBoneInfo.Num());
	for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.RawRefBoneInfo.Num(); ++BoneIndex)
	{
		const int32 ParentIndex = ReferenceSkeleton.RawRefBoneInfo[BoneIndex].ParentIndex;
		if (ParentIndex >= 0)
		{
			Children[ParentIndex].Add(BoneIndex);
		}
	}
	PreviewPose.UpdateComponentSpace();

	FVector Min{ FLT_MAX, FLT_MAX, FLT_MAX };
	FVector Max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const auto& Bone : PreviewPose.ComponentSpacePose)
	{
		const FVector Location = Bone.GetOrigin();
		Min.X = std::min(Min.X, Location.X);
		Max.X = std::max(Max.X, Location.X);
		Min.Y = std::min(Min.Y, Location.Y);
		Max.Y = std::max(Max.Y, Location.Y);
		Min.Z = std::min(Min.Z, Location.Z);
		Max.Z = std::max(Max.Z, Location.Z);
	}
	ViewportClient.SetViewRotation(FVector{ 20.0f, 0.0f, 0.0f });
	ViewportClient.FocusBounds({ Min, Max });

	CameraSpeed = std::max(1.0f, (Max - Min).Length() * 0.5f);

	UndoStack.Reset();
	RedoStack.Reset();
	LastSnapshot = PreviewPose.LocalPose;

	SelectBone(-1);
}

void FSkeletalMeshEditor::SelectBone(int32 BoneIndex)
{
	BoneGizmo.EndDrag();
	if (!PreviewPose.LocalPose.IsValidIndex(BoneIndex)) BoneIndex = -1;
	SelectedBoneIndex = BoneIndex;
	BoneGizmoTarget.SetBone(&PreviewPose, BoneIndex);
	BoneGizmo.SetTarget(BoneIndex >= 0 ? &BoneGizmoTarget : nullptr);
}

int32 FSkeletalMeshEditor::PickBone(const FVector2& MousePosition)
{
	TArray<float> JointRadii = GetJointRadii(PreviewPose);

	const auto& Pose = PreviewPose.ComponentSpacePose;
	const FMatrix ViewProjection = ViewportClient.GetViewProjectionMatrix();
	const FVector CameraRight = ViewportClient.GetViewRotation().Quaternion().GetRightVector();
	const int32 Width = static_cast<int32>(ViewportClient.GetWidth());
	const int32 Height = static_cast<int32>(ViewportClient.GetHeight());

	const auto GetHitRadius = [&](const FVector& WorldLocation, const FVector2& ScreenLocation, float WorldRadius) -> float
		{
			constexpr float MinHitPixels = 6.0f;
			const FVector2 Edge = WorldToScreen(WorldLocation + CameraRight * WorldRadius, ViewProjection, Width, Height);
			const float HitPixels = (Edge - ScreenLocation).Length();
			return std::max(HitPixels, MinHitPixels);
		};

	TArray<FVector2> ScreenJoints;
	ScreenJoints.SetNum(Pose.Num());
	for (int32 Index = 0; Index < Pose.Num(); ++Index)
	{
		ScreenJoints[Index] = WorldToScreen(Pose[Index].GetOrigin(), ViewProjection, Width, Height);
	}

	// Joint
	float MinDistance = FLT_MAX;
	int32 PickedJoint = -1;
	for (int32 Index = 0; Index < Pose.Num(); ++Index)
	{
		if (ScreenJoints[Index].X == -FLT_MAX)
		{
			continue;
		}

		const float Distance = (MousePosition - ScreenJoints[Index]).Length();
		const float HitRadius = GetHitRadius(Pose[Index].GetOrigin(), ScreenJoints[Index], JointRadii[Index]);
		if (Distance <= HitRadius && Distance < MinDistance)
		{
			MinDistance = Distance;
			PickedJoint = Index;
		}
	}
	if (PickedJoint >= 0)
	{
		return PickedJoint;
	}

	// Bone
	MinDistance = FLT_MAX;
	int32 PickedBone = -1;
	for (int32 Index = 0; Index < Pose.Num(); ++Index)
	{
		const int32 ParentIndex = ReferenceSkeleton.RawRefBoneInfo[Index].ParentIndex;
		if (ParentIndex < 0 || ScreenJoints[Index].X == -FLT_MAX || ScreenJoints[ParentIndex].X == -FLT_MAX)
		{
			continue;
		}

		const FVector ParentLocation = Pose[ParentIndex].GetOrigin();
		const FVector ChildLocation = Pose[Index].GetOrigin();
		const FVector Middle = (ParentLocation + ChildLocation) * 0.5f;
		const float WorldRadius = (ChildLocation - ParentLocation).Length() * 0.1f;

		const float Distance = DistanceToSegment(MousePosition, ScreenJoints[ParentIndex], ScreenJoints[Index]);
		const float HitRadius = GetHitRadius(Middle, WorldToScreen(Middle, ViewProjection, Width, Height), WorldRadius);
		if (Distance <= HitRadius && Distance < MinDistance)
		{
			MinDistance = Distance;
			PickedBone = ParentIndex;
		}
	}

	return PickedBone;
}

void FSkeletalMeshEditor::CommitUndo()
{
	bool bChanged = false;
	TArray<FBoneChange> Changes;
	for (int32 Index = 0; Index < PreviewPose.LocalPose.Num(); ++Index)
	{
		if (PreviewPose.LocalPose[Index] != LastSnapshot[Index])
		{
			FBoneChange Change;
			Change.BoneIndex = Index;
			Change.Before = LastSnapshot[Index];
			Change.After = PreviewPose.LocalPose[Index];
			Changes.Push(Change);
			LastSnapshot[Index] = PreviewPose.LocalPose[Index];
			bChanged = true;
		}
	}

	if (bChanged)
	{
		RedoStack.Reset();
		UndoStack.Push(Changes);
	}
}

void FSkeletalMeshEditor::Undo()
{
	CommitUndo();
	if (UndoStack.Num() > 0)
	{
		const TArray<FBoneChange> Changes = UndoStack.Last();
		UndoStack.Pop();

		for (const FBoneChange& Change : Changes)
		{
			PreviewPose.LocalPose[Change.BoneIndex] = Change.Before;
		}
		RedoStack.Push(Changes);

		for (const FBoneChange& Change : Changes)
		{
			LastSnapshot[Change.BoneIndex] = Change.Before;
		}
	}
}

void FSkeletalMeshEditor::Redo()
{
	CommitUndo();
	if (RedoStack.Num() > 0)
	{
		const TArray<FBoneChange> Changes = RedoStack.Last();
		RedoStack.Pop();

		for (const FBoneChange& Change : Changes)
		{
			PreviewPose.LocalPose[Change.BoneIndex] = Change.After;
		}
		UndoStack.Push(Changes);

		for (const FBoneChange& Change : Changes)
		{
			LastSnapshot[Change.BoneIndex] = Change.After;
		}
	}
}

void FSkeletalMeshEditor::SyncPoseToComponent()
{
	if (!PreviewMeshComponent || PreviewMeshComponent->GetNumBones() != PreviewPose.LocalPose.Num())
	{
		return;
	}

	for (int32 Index = 0; Index < PreviewPose.LocalPose.Num(); ++Index)
	{
		if (PreviewPose.LocalPose[Index] != PreviewMeshComponent->GetBoneLocalTransform(Index))
		{
			PreviewMeshComponent->SetBoneLocalTransform(Index, PreviewPose.LocalPose[Index]);
		}
	}
}

