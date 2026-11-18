#include "EnginePCH.h"
#include "Editor/AnimationSequenceEditor/AnimationSequenceEditor.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/ViewportCameraMovement.h"
#include "Editor/Settings/SettingsPanel.h"
#include "Engine/Classes/Animation/AnimSequence.h"
#include "Engine/Classes/Animation/BonePose.h"
#include "Render/SkeletalMesh.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "Editor/AnimationSequenceEditor/AnimationSequenceEditorTab.h"

FAnimationSequenceEditor::FAnimationSequenceEditor(UEngine* InEngine, FEditorManager& InManager,
	const FAssetEditorKey& InKey, UAnimSequence* InSequence, USkeletalMesh* InMesh,
	const FEditorSettings* InSettings, FPreviewRenderer InRenderer)
	: FAssetEditor(InManager, InKey), Engine(InEngine), InitialSequence(InSequence), InitialMesh(InMesh),
	PreviewRenderer(std::move(InRenderer))
{
	Settings = InSettings;
}

TUniquePtr<IEditorTab> FAnimationSequenceEditor::CreateTab()
{
	return MakeUnique<FAnimationSequenceEditorTab>(*this);
}

void FAnimationSequenceEditor::RenderPreview()
{
	if (bViewportVisible && !IsClosing() && PreviewRenderer) PreviewRenderer(*this);
}

bool FAnimationSequenceEditor::InitializeResources()
{
	if (PreviewScene.GetWorld()) return true;

	FPreviewSceneOptions Options;
	Options.WorldName = "AnimationSequencePreview";
	// 포즈는 시간 변경 시 수동 평가하고, 씬은 변환만 갱신한다.
	Options.bSimulate = false;
	Options.SkyLightIntensity = 1.0f;
	Options.DirectionalLightIntensity = 3.0f;
	if (!PreviewScene.Initialize(Engine, Options)) return false;

	ViewportClient.SetWorld(PreviewScene.GetWorld());
	ViewportClient.SetViewLocation(FVector(-4.0f, 0.0f, 2.0f));
	ViewportClient.SetViewRotation(FRotator(20.0f, 0.0f, 0.0f));
	PreviewMeshComponent = Cast<USkeletalMeshComponent>(
		PreviewScene.AddComponent(USkeletalMeshComponent::StaticClass(), "PreviewMesh"));
	if (!PreviewMeshComponent)
	{
		ShutdownResources();
		return false;
	}
	return OpenAsset(InitialSequence, InitialMesh);
}

void FAnimationSequenceEditor::Tick(float DeltaTime)
{
	// 숨겨진 탭은 시간을 유지하며, 분리된 표시 패널은 재생을 계속한다.
	if (bVisible && !IsClosing() && bPlaying && EditSequence && std::isfinite(DeltaTime) && DeltaTime > 0.0f)
	{
		const float Length = EditSequence->GetPlayLength();
		if (!std::isfinite(Length) || Length <= 0.0f)
		{
			bPlaying = false;
		}
		else
		{
			double NewTime = static_cast<double>(CurrentTime) + static_cast<double>(DeltaTime) * PlayRate;
			if (NewTime >= Length)
			{
				if (bLooping)
				{
					// 끝을 초과한 시간까지 유지하며 다음 반복으로 넘어간다.
					NewTime = std::fmod(NewTime, static_cast<double>(Length));
				}
				else
				{
					NewTime = Length;
					bPlaying = false;
				}
			}
			SetPreviewTime(static_cast<float>(NewTime));
		}
	}

	PreviewScene.Tick(DeltaTime);
}

bool FAnimationSequenceEditor::OpenAsset(UAnimSequence* InSequence, USkeletalMesh* InSkeletalMesh)
{
	if (!PreviewScene.GetWorld() || !PreviewMeshComponent || !InSequence || !InSkeletalMesh)
	{
		HTR_LOG(Error, "Cannot open animation sequence: preview or asset is missing.");
		return false;
	}

	const UAnimDataModel* Model = InSequence->GetDataModel();
	if (!Model || Model->GetNumberOfKeys() <= 0 || InSkeletalMesh->GetRefSkeleton().RawRefBoneInfo.IsEmpty())
	{
		HTR_LOG(Error, "Cannot open animation sequence '{}': animation keys or mesh skeleton are missing.", InSequence->GetPath());
		return false;
	}

	PreviewMeshComponent->SetSkeletalMesh(InSkeletalMesh);
	if (PreviewMeshComponent->GetSkeletalMesh() != InSkeletalMesh) return false;
	// 클립/메시 교체 시 이전 본 선택을 지우고, 변하지 않는 계층은 한 번만 구성한다.
	SelectedBoneIndex = -1;
	const auto& Bones = InSkeletalMesh->GetRefSkeleton().RawRefBoneInfo;
	BoneChildren.Reset();
	BoneChildren.SetNum(Bones.Num());
	for (int32 BoneIndex = 0; BoneIndex < Bones.Num(); ++BoneIndex)
	{
		const int32 ParentIndex = Bones[BoneIndex].ParentIndex;
		if (BoneChildren.IsValidIndex(ParentIndex))
			BoneChildren[ParentIndex].Add(BoneIndex);
	}
	PreviewMeshComponent->ResetToReferencePose();
	EditSequence = InSequence;
	bPlaying = false;
	SelectedNotifyIndex = INDEX_NONE;
	DraggingNotifyIndex = INDEX_NONE;
	DragMode = ENotifyDragMode::None;
	EditNotifyName[0] = '\0';
	SetPreviewTime(0.0f);
	bFitTimeline = true;

	const FBox Bounds = PreviewMeshComponent->CalcBounds();
	ViewportClient.FocusBounds(Bounds);
	CameraSpeed = std::max(1.0f, (Bounds.Max - Bounds.Min).Length() * 0.5f);
	return true;
}

int32 FAnimationSequenceEditor::AddNotifyEvent(const char* InName, float InTriggerTime, float InDuration)
{
	if (!EditSequence || !std::isfinite(InTriggerTime) || !std::isfinite(InDuration)) return INDEX_NONE;
	const float Length = EditSequence->GetPlayLength();
	FAnimNotifyEvent NewEvent;
	NewEvent.Duration = std::max(0.0f, std::min(InDuration, Length));
	NewEvent.TriggerTime = std::clamp(InTriggerTime, 0.0f, std::max(0.0f, Length - NewEvent.Duration));
	NewEvent.NotifyName = FName((InName && InName[0] != '\0') ? InName : "AnimNotify");
	const int32 NewIdx = EditSequence->Notifies.Add(NewEvent);
	SelectNotify(NewIdx);
	return NewIdx;
}

void FAnimationSequenceEditor::SelectNotify(int32 Index)
{
	if (!EditSequence || !EditSequence->Notifies.IsValidIndex(Index))
	{
		SelectedNotifyIndex = INDEX_NONE;
		EditNotifyName[0] = '\0';
		return;
	}

	SelectedNotifyIndex = Index;
	const FString Name = EditSequence->Notifies[Index].NotifyName.ToString();
	std::snprintf(EditNotifyName, sizeof(EditNotifyName), "%s", Name.c_str());
}

void FAnimationSequenceEditor::TogglePlayback()
{
	if (!EditSequence) return;
	const float Length = EditSequence->GetPlayLength();
	if (!std::isfinite(Length) || Length <= 0.0f) return;

	if (!bPlaying && CurrentTime >= Length) SetPreviewTime(0.0f);
	bPlaying = !bPlaying;
}

void FAnimationSequenceEditor::StepFrame(int32 Direction)
{
	if (!EditSequence) return;
	bPlaying = false;
	const double FPS = EditSequence->GetSamplingFrameRate().AsDecimal();
	if (!std::isfinite(FPS) || FPS <= 0.0) return;

	const double CurrentFrame = std::round(static_cast<double>(CurrentTime) * FPS);
	SetPreviewTime(static_cast<float>((CurrentFrame + Direction) / FPS));
}

void FAnimationSequenceEditor::SetPreviewTime(float NewTime)
{
	if (!EditSequence || !std::isfinite(NewTime)) return;
	const float Length = EditSequence->GetPlayLength();
	if (!std::isfinite(Length) || Length < 0.0f) return;

	CurrentTime = std::clamp(NewTime, 0.0f, Length);
	EvaluatePreviewPose();
}

void FAnimationSequenceEditor::EvaluatePreviewPose()
{
	if (!EditSequence || !PreviewMeshComponent) return;
	USkeletalMesh* Mesh = PreviewMeshComponent->GetSkeletalMesh();
	if (!Mesh) return;

	// 메시의 스켈레톤을 기준으로 현재 시간의 본별 로컬 자세를 구한다.
	FCompactPose Pose;
	Pose.SetRefSkeleton(&Mesh->GetRefSkeleton());
	FAnimExtractContext Context;
	Context.CurrentTime = CurrentTime;
	Context.bLooping = false;
	EditSequence->GetAnimationPose(Pose, Context);

	if (Pose.GetNumBones() != PreviewMeshComponent->GetNumBones()) return;
	for (int32 BoneIndex = 0; BoneIndex < Pose.GetNumBones(); ++BoneIndex)
	{
		PreviewMeshComponent->SetBoneLocalTransform(BoneIndex, Pose[BoneIndex]);
	}
}

bool FAnimationSequenceEditor::IsBoneWeightEnabled() const
{
	return bShowBoneWeight && USkeletalMeshComponent::IsGPUSkinningEnabled() &&
		PreviewMeshComponent && PreviewMeshComponent->GetSkeletalMesh();
}

int32 FAnimationSequenceEditor::GetBoneWeightBoneIndex() const
{
	if (!IsBoneWeightEnabled() ||
		!PreviewMeshComponent->GetSkeletalMesh()->GetRefSkeleton().RawRefBoneInfo.IsValidIndex(SelectedBoneIndex))
		return -1;
	return SelectedBoneIndex;
}

void FAnimationSequenceEditor::ShutdownResources()
{
	// 월드를 삭제하기 전에 관찰 참조부터 해제
	ViewportClient.SetWorld(nullptr);
	bPlaying = false;
	EditSequence = nullptr;
	SelectedNotifyIndex = INDEX_NONE;
	DraggingNotifyIndex = INDEX_NONE;
	DragMode = ENotifyDragMode::None;
	EditNotifyName[0] = '\0';
	CurrentTime = 0.0f;
	TimelineView = {};
	bFitTimeline = true;
	SelectedBoneIndex = -1;
	BoneChildren.Reset();
	PreviewMeshComponent = nullptr;

	PreviewScene.Release();
}

void FAnimationSequenceEditor::SetPlayRate(float Value)
{
	if (std::isfinite(Value)) PlayRate = std::clamp(Value, 0.1f, 4.0f);
}

void FAnimationSequenceEditor::SelectBone(int32 Index)
{
	SelectedBoneIndex = BoneChildren.IsValidIndex(Index) ? Index : INDEX_NONE;
}

void FAnimationSequenceEditor::RemoveNotify(int32 Index)
{
	if (!EditSequence || !EditSequence->Notifies.IsValidIndex(Index)) return;
	EditSequence->Notifies.RemoveAt(Index);
	SelectNotify(INDEX_NONE);
	DraggingNotifyIndex = INDEX_NONE;
	DragMode = ENotifyDragMode::None;
}

void FAnimationSequenceEditor::UpdateNotify(int32 Index, const char* Name, float Time, float Duration)
{
	if (!EditSequence || !EditSequence->Notifies.IsValidIndex(Index) ||
		!std::isfinite(Time) || !std::isfinite(Duration)) return;
	FAnimNotifyEvent& Notify = EditSequence->Notifies[Index];
	const float Length = EditSequence->GetPlayLength();
	Notify.TriggerTime = std::clamp(Time, 0.0f, Length);
	Notify.Duration = std::clamp(Duration, 0.0f, Length - Notify.TriggerTime);
	Notify.NotifyName = FName(Name && Name[0] ? Name : "AnimNotify");
}

void FAnimationSequenceEditor::ProcessCameraInput(const FViewportCameraInput& Input, float DeltaTime)
{
	ViewportClient.ApplyCameraInput(Input, DeltaTime, CameraSpeed * (Settings ? Settings->CameraSpeed : 1.0f),
		0.2f * (Settings ? Settings->MouseSensitivity : 1.0f));
}
