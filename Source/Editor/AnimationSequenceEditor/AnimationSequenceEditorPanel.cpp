#include "EnginePCH.h"
#include "Editor/AnimationSequenceEditor/AnimationSequenceEditorPanel.h"

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

FAnimationSequenceEditorPanel::FAnimationSequenceEditorPanel(FAnimationSequenceEditor& InEditor, ERole InRole, const char* InName)
	: Editor(InEditor), Role(InRole), PanelName(InName)
{
	WindowName = PanelName + "###AnimationPanel:" + PanelName + ":" + Editor.GetKey().GetTabId();
}

bool FAnimationSequenceEditorPanel::IsDetached(ImGuiID DockSpaceId) const
{
	const ImGuiWindow* Window = ImGui::FindWindowByName(GetWindowName());
	return Window && (!Window->DockNode || ImGui::DockNodeGetRootNode(Window->DockNode)->ID != DockSpaceId);
}

void FAnimationSequenceEditorPanel::OnRender()
{
	ImGuiWindowClass WindowClass;
	WindowClass.ClassId = DockClassId;
	WindowClass.DockingAllowUnclassed = false;
	ImGui::SetNextWindowClass(&WindowClass);
	bool bOpen = IsOpen();
	if (ImGui::Begin(GetWindowName(), &bOpen))
	{
		Editor.SetPanelVisible();
		Editor.ReportPanelFocus(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
		switch (Role)
		{
		case ERole::Viewport:
			Editor.SetViewportVisible();
			ImGui::Checkbox("Grid", &Editor.bRenderGrid);
			ImGui::SameLine();
			ImGui::Checkbox("Bone Weight", &Editor.bShowBoneWeight);
			DrawViewport();
			break;
		case ERole::Hierarchy: DrawBoneHierarchy(); break;
		case ERole::SequenceInfo: DrawSequenceInfo(); break;
		case ERole::Timeline:
			DrawPlaybackControls();
			DrawAnimationTimeline();
			break;
		case ERole::NotifyDetails: DrawSelectedNotifyInspector(); break;
		}
	}
	ImGui::End();
	SetOpen(bOpen);
}

void FAnimationSequenceEditorPanel::DrawViewport()
{
	if (Editor.IsBoneWeightEnabled())
	{
		const int32 BoneIndex = Editor.GetBoneWeightBoneIndex();
		if (BoneIndex >= 0)
			ImGui::TextWrapped("Bone Weight: %s",
				Editor.PreviewMeshComponent->GetSkeletalMesh()->GetRefSkeleton().RawRefBoneInfo[BoneIndex].Name.ToString().c_str());
		else
			ImGui::TextDisabled("No bone selected: Weight 0 (Purple).");
		ImGui::TextDisabled("0 Purple > Blue > Green > Yellow > Red 1");
	}

	const ImVec2 ViewportSize = ImGui::GetContentRegionAvail();
	const uint32 Width = static_cast<uint32>(std::max(1.0f, ViewportSize.x));
	const uint32 Height = static_cast<uint32>(std::max(1.0f, ViewportSize.y));
	if (Width != Editor.ViewportClient.GetWidth() || Height != Editor.ViewportClient.GetHeight())
	{
		Editor.ViewportClient.Resize(Width, Height);
	}
	if (Editor.ViewportClient.GetColorTarget() && Editor.ViewportClient.GetColorTarget()->GetSRV())
	{
		const ImVec2 Min = ImGui::GetCursorScreenPos();
		const ImVec2 Size(static_cast<float>(Width), static_cast<float>(Height));
		ImGui::InvisibleButton("##AnimationPreviewViewport", Size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		ImGui::GetWindowDrawList()->AddImage(Editor.ViewportClient.GetColorTarget()->GetSRV(), Min, ImVec2(Min.x + Size.x, Min.y + Size.y));
		TickCameraInput();
		// 프리뷰 클릭은 선택만 해제한다. Heatmap 모드는 유지해 메시를 보라색으로 표시한다.
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			Editor.SelectBone(INDEX_NONE);
	}
}

void FAnimationSequenceEditorPanel::TickCameraInput()
{
	if (ImGui::IsItemActivated())
		ImGui::SetWindowFocus();
	const ImGuiIO& IO = ImGui::GetIO();
	const bool bCaptured = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Right) ;

	FViewportCameraInput Input{
		.bCaptured = bCaptured,
		.bAllowWheel = bCaptured || ImGui::IsItemHovered(),
		.bForward = ImGui::IsKeyDown(ImGuiKey_W),
		.bBackward = ImGui::IsKeyDown(ImGuiKey_S),
		.bRight = ImGui::IsKeyDown(ImGuiKey_D),
		.bLeft = ImGui::IsKeyDown(ImGuiKey_A),
		.bUp = ImGui::IsKeyDown(ImGuiKey_E),
		.bDown = ImGui::IsKeyDown(ImGuiKey_Q),
		.MouseX = static_cast<float>(IO.MouseDelta.x),
		.MouseY = static_cast<float>(IO.MouseDelta.y),
		.Wheel = static_cast<float>(IO.MouseWheel) * 120.0f,
	};

	Editor.ProcessCameraInput(Input, IO.DeltaTime);
}

void FAnimationSequenceEditorPanel::DrawBoneHierarchy()
{
	ImGui::SeparatorText("Bone Hierarchy");
	const USkeletalMesh* Mesh = Editor.PreviewMeshComponent ? Editor.PreviewMeshComponent->GetSkeletalMesh() : nullptr;
	if (!Mesh)
	{
		ImGui::TextDisabled("Open an animation to view its skeleton.");
		return;
	}

	ImGui::BeginDisabled(Editor.SelectedBoneIndex < 0);
	if (ImGui::Button("Clear Selection")) Editor.SelectBone(INDEX_NONE);
	ImGui::EndDisabled();

	const auto& Bones = Mesh->GetRefSkeleton().RawRefBoneInfo;
	for (int32 BoneIndex = 0; BoneIndex < Bones.Num(); ++BoneIndex)
	{
		if (Bones[BoneIndex].ParentIndex < 0)
			DrawBoneTreeNode(BoneIndex);
	}
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		Editor.SelectBone(INDEX_NONE);
}

void FAnimationSequenceEditorPanel::DrawBoneTreeNode(int32 BoneIndex)
{
	const USkeletalMesh* Mesh = Editor.PreviewMeshComponent ? Editor.PreviewMeshComponent->GetSkeletalMesh() : nullptr;
	if (!Mesh || !Editor.BoneChildren.IsValidIndex(BoneIndex)) return;
	const auto& Bones = Mesh->GetRefSkeleton().RawRefBoneInfo;
	if (!Bones.IsValidIndex(BoneIndex)) return;

	const bool bIsLeaf = Editor.BoneChildren[BoneIndex].IsEmpty();
	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
	if (bIsLeaf) Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (BoneIndex == Editor.SelectedBoneIndex) Flags |= ImGuiTreeNodeFlags_Selected;

	const FString Name = Bones[BoneIndex].Name.ToString();
	ImGui::PushID(BoneIndex);
	const bool bOpen = ImGui::TreeNodeEx("##Bone", Flags, "%s", Name.c_str());
	ImGui::PopID();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s (Index: %d)", Name.c_str(), BoneIndex);
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
		Editor.SelectBone(BoneIndex);

	if (bOpen && !bIsLeaf)
	{
		for (int32 ChildIndex : Editor.BoneChildren[BoneIndex])
			DrawBoneTreeNode(ChildIndex);
		ImGui::TreePop();
	}
}

void FAnimationSequenceEditorPanel::DrawSequenceInfo()
{
	if (!Editor.EditSequence)
	{
		ImGui::TextUnformatted("No animation sequence selected.");
		return;
	}

	const FString& Key = Editor.EditSequence->GetPath();
	const size_t Separator = Key.find_last_of('/');
	const FString Name = Separator == FString::npos ? Key : Key.substr(Separator + 1);
	ImGui::Text("Animation: %s", Name.c_str());
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Key.c_str());
	ImGui::SameLine(0.0f, 16.0f);
	ImGui::TextDisabled("Length: %.3f sec | FPS: %.2f | Keys: %d",
		Editor.EditSequence->GetPlayLength(), Editor.EditSequence->GetSamplingFrameRate().AsDecimal(), Editor.EditSequence->GetNumberOfSampledKeys());

}

void FAnimationSequenceEditorPanel::DrawSelectedNotifyInspector()
{
	if (!Editor.EditSequence) return;

	const float Length = Editor.EditSequence->GetPlayLength();

	ImGui::SetNextItemWidth(120.0f);
	ImGui::InputText("##QuickNotifyName", Editor.NewNotifyName, sizeof(Editor.NewNotifyName));
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("New AnimNotify Name");
	ImGui::SetNextItemWidth(65.0f);
	ImGui::DragFloat("##QuickNotifyDur", &Editor.NewNotifyDuration, 0.01f, 0.0f, Length, "%.2fs", ImGuiSliderFlags_AlwaysClamp);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duration (0.00 = Point Notify, > 0 = Notify State)");
	if (ImGui::Button("+ Add @ Playhead"))
	{
		Editor.AddNotifyEvent(Editor.NewNotifyName, Editor.CurrentTime, Editor.NewNotifyDuration);
	}

	if (Editor.EditSequence->Notifies.IsValidIndex(Editor.SelectedNotifyIndex))
	{
		FAnimNotifyEvent& Notify = Editor.EditSequence->Notifies[Editor.SelectedNotifyIndex];
		ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.2f, 1.0f), "Selected [%d]:", Editor.SelectedNotifyIndex);
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::InputText("Name##EditNotify", Editor.EditNotifyName, sizeof(Editor.EditNotifyName)))
		{
			Editor.UpdateNotify(Editor.SelectedNotifyIndex, Editor.EditNotifyName, Notify.TriggerTime, Notify.Duration);
		}
		ImGui::SetNextItemWidth(75.0f);
		const float MaxStartTime = std::max(0.0f, Length - Notify.Duration);
		float Time = Notify.TriggerTime;
		if (ImGui::DragFloat("Time##EditNotify", &Time, 0.005f, 0.0f, MaxStartTime, "%.3fs", ImGuiSliderFlags_AlwaysClamp))
			Editor.UpdateNotify(Editor.SelectedNotifyIndex, Editor.EditNotifyName, Time, Notify.Duration);
		ImGui::SetNextItemWidth(75.0f);
		float Duration = Notify.Duration;
		if (ImGui::DragFloat("Dur##EditNotify", &Duration, 0.005f, 0.0f, std::max(0.0f, Length - Notify.TriggerTime), "%.3fs", ImGuiSliderFlags_AlwaysClamp))
			Editor.UpdateNotify(Editor.SelectedNotifyIndex, Editor.EditNotifyName, Notify.TriggerTime, Duration);
		if (ImGui::Button("Delete##EditNotify"))
		{
			Editor.RemoveNotify(Editor.SelectedNotifyIndex);
		}
	}
	else
	{
		ImGui::TextDisabled("(Right-click Notify track to add / Left-click & drag marker to move)");
	}
}

void FAnimationSequenceEditorPanel::DrawPlaybackControls()
{
	if (!Editor.EditSequence)
	{
		ImGui::TextDisabled("Open an animation from the Content Drawer.");
		return;
	}
	const float Length = Editor.EditSequence->GetPlayLength();
	if (!std::isfinite(Length) || Length < 0.0f) return;
	if (!ImGui::BeginTable("##PlaybackControls", 2, ImGuiTableFlags_SizingStretchSame)) return;

	ImGui::TableNextColumn();
	if (ImGui::Button("|<"))
	{
		Editor.bPlaying = false;
		Editor.SetPreviewTime(0.0f);
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go to start");
	ImGui::SameLine();
	if (ImGui::Button("<")) Editor.StepFrame(-1);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Previous frame");
	ImGui::SameLine();
	ImGui::BeginDisabled(Length <= 0.0f);
	if (ImGui::Button(Editor.bPlaying ? "Pause" : "Play", { 55.0f, 0.0f })) Editor.TogglePlayback();
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button(">")) Editor.StepFrame(+1);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Next frame");
	ImGui::SameLine();
	if (ImGui::Button(">|"))
	{
		Editor.bPlaying = false;
		Editor.SetPreviewTime(Length);
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go to end");

	ImGui::TableNextColumn();
	ImGui::Checkbox("Loop", &Editor.bLooping);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70.0f);
	ImGui::DragFloat("Speed", &Editor.PlayRate, 0.05f, 0.1f, 4.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);

	ImGui::TableNextColumn();
	ImGui::SetNextItemWidth(100.0f);
	float EditedTime = Editor.CurrentTime;
	const bool bInputChanged = ImGui::InputFloat("Time (sec)", &EditedTime, 0.01f, 0.1f, "%.3f");
	if (ImGui::IsItemActivated() || bInputChanged) Editor.bPlaying = false;
	if (bInputChanged) Editor.SetPreviewTime(EditedTime);

	ImGui::TableNextColumn();
	if (ImGui::Button("Fit")) Editor.bFitTimeline = true;
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fit the entire animation in the timeline");
	ImGui::SameLine();
	const double FPS = Editor.EditSequence->GetSamplingFrameRate().AsDecimal();
	if (std::isfinite(FPS) && FPS > 0.0)
		ImGui::Text("Frame %.0f / %d", std::round(Editor.CurrentTime * FPS), std::max(0, Editor.EditSequence->GetNumberOfSampledKeys() - 1));
	ImGui::EndTable();
}

void FAnimationSequenceEditorPanel::DrawAnimationTimeline()
{
	if (!Editor.EditSequence)
	{
		ImGui::TextDisabled("No animation sequence selected.");
		return;
	}
	const float Length = Editor.EditSequence->GetPlayLength();
	const double FPS = Editor.EditSequence->GetSamplingFrameRate().AsDecimal();
	if (!std::isfinite(Length) || Length <= 0.0f || !std::isfinite(FPS) || FPS <= 0.0)
	{
		ImGui::TextDisabled("No timeline range available.");
		return;
	}

	const ImVec2 Origin = ImGui::GetCursorScreenPos();
	const ImVec2 Size = ImGui::GetContentRegionAvail();
	const float LabelWidth = std::min(160.0f, Size.x * 0.3f);
	constexpr float RulerHeight = 26.0f;
	constexpr float HeaderHeight = 24.0f;
	constexpr float LaneHeight = 40.0f;
	if (Size.x <= LabelWidth + 16.0f || Size.y < RulerHeight + HeaderHeight + LaneHeight) return;

	const ImVec2 AxisMin{ Origin.x + LabelWidth, Origin.y };
	const ImVec2 AxisMax{ Origin.x + Size.x, Origin.y + Size.y };
	const float AxisWidth = AxisMax.x - AxisMin.x;
	const float FitScale = AxisWidth / Length;
	if (Editor.bFitTimeline)
	{
		Editor.TimelineView.StartTime = 0.0f;
		Editor.TimelineView.PixelsPerSecond = FitScale;
		Editor.bFitTimeline = false;
	}
	Editor.TimelineView.PixelsPerSecond = std::max(Editor.TimelineView.PixelsPerSecond, FitScale);

	const float HeaderY = Origin.y + RulerHeight;
	const float LaneTop = HeaderY + HeaderHeight;
	const float LaneBottom = LaneTop + LaneHeight;
	const ImVec2 LaneMin{ AxisMin.x, LaneTop };
	const ImVec2 LaneMax{ AxisMax.x, LaneBottom };

	ImGui::SetCursorScreenPos(AxisMin);
	ImGui::InvisibleButton("##AnimationTimelineInput", { AxisWidth, Size.y });
	const ImGuiIO& IO = ImGui::GetIO();
	const bool bInputHovered = ImGui::IsItemHovered();
	const bool bInputActive = ImGui::IsItemActive();

	if (bInputHovered)
	{
		if (IO.KeyCtrl && IO.MouseWheel != 0.0f)
			Editor.TimelineView.ZoomAt(IO.MouseWheel > 0.0f ? 1.15f : 1.0f / 1.15f, IO.MousePos.x, AxisMin.x, FitScale, FitScale * 100.0f);
		else if (IO.KeyShift && IO.MouseWheel != 0.0f)
			Editor.TimelineView.PanPixels(-IO.MouseWheel * 40.0f);
	}
	const float VisibleDuration = AxisWidth / Editor.TimelineView.PixelsPerSecond;
	Editor.TimelineView.StartTime = std::clamp(Editor.TimelineView.StartTime, 0.0f, std::max(0.0f, Length - VisibleDuration));

	const bool bInNotifyLane = (IO.MousePos.y >= LaneTop && IO.MousePos.y <= LaneBottom);

	// 호버 시 리사이즈 핸들 커서 표시
	if (Editor.DraggingNotifyIndex == INDEX_NONE && bInputHovered && bInNotifyLane)
	{
		ENotifyDragMode HoverPart = ENotifyDragMode::None;
		if (HitTestNotify(IO.MousePos.x, IO.MousePos.y, LaneMin, LaneMax, &HoverPart) != INDEX_NONE &&
			HoverPart == ENotifyDragMode::ResizeEnd)
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
	}

	// 좌클릭 시작: 노티파이 트랙 내 마커/구간 핸들 히트 테스트
	if (bInputHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		if (bInNotifyLane)
		{
			ENotifyDragMode HitPart = ENotifyDragMode::None;
			const int32 HitIdx = HitTestNotify(IO.MousePos.x, IO.MousePos.y, LaneMin, LaneMax, &HitPart);
			if (HitIdx != INDEX_NONE)
			{
				Editor.SelectNotify(HitIdx);
				Editor.DraggingNotifyIndex = HitIdx;
				Editor.DragMode = HitPart;
			}
			else
			{
				Editor.SelectNotify(INDEX_NONE);
				Editor.DraggingNotifyIndex = INDEX_NONE;
				Editor.DragMode = ENotifyDragMode::None;
			}
		}
		else
		{
			Editor.DraggingNotifyIndex = INDEX_NONE;
			Editor.DragMode = ENotifyDragMode::None;
		}
	}

	// 좌클릭 드래그: 노티파이 이동, 구간 끝 리사이즈, 또는 플레이헤드 스크러빙
	if (bInputActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		if (Editor.DraggingNotifyIndex != INDEX_NONE && Editor.EditSequence->Notifies.IsValidIndex(Editor.DraggingNotifyIndex))
		{
			FAnimNotifyEvent& DragNotify = Editor.EditSequence->Notifies[Editor.DraggingNotifyIndex];
			float MouseTime = std::clamp(Editor.TimelineView.XToTime(IO.MousePos.x, AxisMin.x), 0.0f, Length);
			if (IO.KeyShift)
			{
				MouseTime = static_cast<float>(std::round(MouseTime * FPS) / FPS);
			}

			if (Editor.DragMode == ENotifyDragMode::ResizeEnd)
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
				const float MinDur = std::min(0.02f, std::max(0.0f, Length - DragNotify.TriggerTime));
				const float EndTime = std::clamp(MouseTime, DragNotify.TriggerTime + MinDur, Length);
				Editor.UpdateNotify(Editor.DraggingNotifyIndex, DragNotify.NotifyName.ToString().c_str(), DragNotify.TriggerTime, EndTime - DragNotify.TriggerTime);
			}
			else
			{
				const float MaxStart = std::max(0.0f, Length - DragNotify.Duration);
				Editor.UpdateNotify(Editor.DraggingNotifyIndex, DragNotify.NotifyName.ToString().c_str(), std::clamp(MouseTime, 0.0f, MaxStart), DragNotify.Duration);
			}
		}
		else
		{
			Editor.bPlaying = false;
			const float MouseTime = Editor.TimelineView.XToTime(IO.MousePos.x, AxisMin.x);
			Editor.SetPreviewTime(static_cast<float>(std::round(MouseTime * FPS) / FPS));
		}
	}

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		Editor.DraggingNotifyIndex = INDEX_NONE;
		Editor.DragMode = ENotifyDragMode::None;
	}

	// 우클릭: 컨텍스트 메뉴 오픈 (노티파이 추가/삭제)
	if (bInputHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		if (bInNotifyLane)
		{
			const int32 HitIdx = HitTestNotify(IO.MousePos.x, IO.MousePos.y, LaneMin, LaneMax);
			if (HitIdx != INDEX_NONE)
			{
				Editor.SelectNotify(HitIdx);
			}
		}
		Editor.PendingNotifyTime = std::clamp(Editor.TimelineView.XToTime(IO.MousePos.x, AxisMin.x), 0.0f, Length);
		ImGui::OpenPopup("##NotifyContextPopup");
	}

	// Delete 키 단축키: 선택된 노티파이 삭제
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !IO.WantTextInput && Editor.EditSequence->Notifies.IsValidIndex(Editor.SelectedNotifyIndex) && ImGui::IsKeyPressed(ImGuiKey_Delete))
	{
		Editor.RemoveNotify(Editor.SelectedNotifyIndex);
	}

	ImDrawList* Draw = ImGui::GetWindowDrawList();
	Draw->AddRectFilled(Origin, AxisMax, IM_COL32(28, 30, 33, 255));
	Draw->AddRectFilled(Origin, { AxisMin.x, AxisMax.y }, IM_COL32(35, 38, 42, 255));
	Draw->AddText({ Origin.x + 6.0f, Origin.y + 5.0f }, IM_COL32(180, 180, 180, 255), "Tracks");
	EditorTimeline::DrawRuler(Draw, Editor.TimelineView, AxisMin, { AxisMax.x, Origin.y + RulerHeight }, AxisMax.y, FPS, true, true);

	Draw->AddRectFilled({ Origin.x, HeaderY }, { AxisMax.x, LaneTop }, IM_COL32(45, 48, 52, 255));
	char TrackLabel[64];
	std::snprintf(TrackLabel, sizeof(TrackLabel), "Notifies (%d)", Editor.EditSequence->Notifies.Num());
	Draw->AddText({ Origin.x + 6.0f, HeaderY + 4.0f }, IM_COL32(220, 220, 220, 255), TrackLabel);
	Draw->AddText({ Origin.x + 18.0f, LaneTop + 10.0f }, IM_COL32(190, 190, 190, 255), "1");
	Draw->AddLine({ Origin.x, LaneBottom }, { AxisMax.x, LaneBottom }, IM_COL32(65, 68, 72, 255));
	Draw->AddLine({ AxisMin.x, Origin.y }, { AxisMin.x, AxisMax.y }, IM_COL32(65, 68, 72, 255));

	Draw->PushClipRect(LaneMin, LaneMax, true);
	DrawNotifyTrack(Draw, LaneMin, LaneMax);
	Draw->PopClipRect();
	EditorTimeline::DrawPlayhead(Draw, Editor.TimelineView, Editor.CurrentTime, AxisMin, AxisMax);

	if (ImGui::BeginPopup("##NotifyContextPopup"))
	{
		ImGui::Text("Time: %.3f sec", Editor.PendingNotifyTime);
		ImGui::Separator();
		ImGui::SetNextItemWidth(160.0f);
		ImGui::InputText("Name##PopupNotify", Editor.NewNotifyName, sizeof(Editor.NewNotifyName));
		ImGui::SetNextItemWidth(160.0f);
		ImGui::DragFloat("Dur##PopupNotify", &Editor.NewNotifyDuration, 0.01f, 0.0f, std::max(0.0f, Length - Editor.PendingNotifyTime), "%.2fs (0=Point)", ImGuiSliderFlags_AlwaysClamp);

		if (ImGui::Button("Add Notify"))
		{
			Editor.AddNotifyEvent(Editor.NewNotifyName, Editor.PendingNotifyTime, Editor.NewNotifyDuration);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Add State (0.2s)"))
		{
			Editor.AddNotifyEvent(Editor.NewNotifyName, Editor.PendingNotifyTime, 0.2f);
			ImGui::CloseCurrentPopup();
		}

		ImGui::Separator();
		ImGui::TextDisabled("Quick Presets");
		if (ImGui::MenuItem("Add 'Footstep' (Point)"))
		{
			Editor.AddNotifyEvent("Footstep", Editor.PendingNotifyTime, 0.0f);
		}
		if (ImGui::MenuItem("Add 'Attack_Slash' (Point)"))
		{
			Editor.AddNotifyEvent("Attack_Slash", Editor.PendingNotifyTime, 0.0f);
		}
		if (ImGui::MenuItem("Add 'Attack_Window' (State 0.25s)"))
		{
			Editor.AddNotifyEvent("Attack_Window", Editor.PendingNotifyTime, 0.25f);
		}
		if (ImGui::MenuItem("Add 'Walk_StepWindow' (State 0.20s)"))
		{
			Editor.AddNotifyEvent("Walk_StepWindow", Editor.PendingNotifyTime, 0.20f);
		}

		if (Editor.EditSequence->Notifies.IsValidIndex(Editor.SelectedNotifyIndex))
		{
			ImGui::Separator();
			if (ImGui::MenuItem("Delete Selected Notify"))
			{
				Editor.RemoveNotify(Editor.SelectedNotifyIndex);
			}
		}
		ImGui::EndPopup();
	}
}

void FAnimationSequenceEditorPanel::DrawNotifyTrack(ImDrawList* Draw, const ImVec2& Min, const ImVec2& Max)
{
	if (Editor.EditSequence->Notifies.IsEmpty())
	{
		Draw->AddText({ Min.x + 8.0f, Min.y + 10.0f }, IM_COL32(130, 130, 130, 255), "Right-click to add AnimNotify");
		return;
	}
	const float Length = Editor.EditSequence->GetPlayLength();
	const float CenterY = Min.y + 12.0f;
	const bool bCanHover = ImGui::IsItemHovered();
	bool bTooltipShown = false;
	for (int32 i = 0; i < Editor.EditSequence->Notifies.Num(); ++i)
	{
		const FAnimNotifyEvent& Notify = Editor.EditSequence->Notifies[i];
		if (!std::isfinite(Notify.TriggerTime) || !std::isfinite(Notify.Duration) ||
			Notify.TriggerTime < 0.0f || Notify.TriggerTime > Length || Notify.Duration < 0.0f) continue;

		const float StartX = Editor.TimelineView.TimeToX(Notify.TriggerTime, Min.x);
		const float EndTime = static_cast<float>(std::min(static_cast<double>(Length), static_cast<double>(Notify.TriggerTime) + Notify.Duration));
		const float EndX = Editor.TimelineView.TimeToX(EndTime, Min.x);
		if (Notify.Duration > 0.0f)
		{
			if (EndX < Min.x || StartX > Max.x) continue;
		}
		else if (StartX < Min.x - 6.0f || StartX > Max.x + 6.0f) continue;

		const FString Name = Notify.NotifyName.ToString();
		const char* Label = Name.empty() ? "(Unnamed)" : Name.c_str();
		const bool bSelected = (i == Editor.SelectedNotifyIndex);
		const ImU32 Color = bSelected ? IM_COL32(255, 195, 50, 255) : IM_COL32(100, 180, 240, 255);

		Draw->AddLine({ StartX, Min.y }, { StartX, Max.y },
			bSelected ? IM_COL32(255, 195, 50, 140) : IM_COL32(100, 180, 240, 80), 1.0f);

		if (Notify.Duration > 0.0f)
		{
			const float Left = std::clamp(StartX, Min.x, Max.x - 2.0f);
			const float Right = std::clamp(EndX, Left + 2.0f, Max.x);
			const ImU32 StateColor = bSelected ? IM_COL32(255, 185, 45, 220) : IM_COL32(75, 175, 130, 210);
			Draw->AddRectFilled({ Left, CenterY - 7.0f }, { Right, CenterY + 7.0f }, StateColor, 2.0f);
			if (EndX >= Min.x && EndX <= Max.x)
			{
				Draw->AddRectFilled({ Right - 3.0f, CenterY - 7.0f }, { Right, CenterY + 7.0f }, IM_COL32(235, 245, 255, 220), 1.0f);
			}
			if (bSelected)
			{
				Draw->AddRect({ Left, CenterY - 7.0f }, { Right, CenterY + 7.0f }, IM_COL32(255, 255, 255, 255), 2.0f, 0, 1.5f);
			}
		}
		else
		{
			Draw->AddQuadFilled({ StartX, CenterY - 6.0f }, { StartX + 6.0f, CenterY },
				{ StartX, CenterY + 6.0f }, { StartX - 6.0f, CenterY }, Color);
			if (bSelected)
			{
				Draw->AddQuad({ StartX, CenterY - 6.0f }, { StartX + 6.0f, CenterY },
					{ StartX, CenterY + 6.0f }, { StartX - 6.0f, CenterY }, IM_COL32(255, 255, 255, 255), 1.5f);
			}
		}
		Draw->AddText({ StartX + 8.0f, CenterY + 10.0f },
			bSelected ? IM_COL32(255, 230, 130, 255) : IM_COL32(220, 220, 220, 255), Label);

		const ImVec2 HitMin{ std::max(Min.x, StartX - 6.0f), CenterY - 8.0f };
		const ImVec2 HitMax{ std::min(Max.x, std::max(EndX + 4.0f, StartX + 6.0f)), CenterY + 8.0f };
		if (bCanHover && !bTooltipShown && ImGui::IsMouseHoveringRect(HitMin, HitMax))
		{
			if (Notify.IsNotifyState())
			{
				ImGui::SetTooltip("%s [NotifyState]\nStart: %.3f sec | Dur: %.3f sec | End: %.3f sec\n(Drag bar to move, Drag right edge to resize, Shift to snap)",
					Label, Notify.TriggerTime, Notify.Duration, EndTime);
			}
			else
			{
				ImGui::SetTooltip("%s [Notify]\nTime: %.3f sec\n(Drag to move, Shift+Drag to snap)", Label, Notify.TriggerTime);
			}
			bTooltipShown = true;
		}
	}
}

int32 FAnimationSequenceEditorPanel::HitTestNotify(float MouseX, float MouseY, const ImVec2& LaneMin, const ImVec2& LaneMax, ENotifyDragMode* OutPart) const
{
	if (OutPart) *OutPart = ENotifyDragMode::None;
	if (!Editor.EditSequence) return INDEX_NONE;
	const float Length = Editor.EditSequence->GetPlayLength();
	const float CenterY = LaneMin.y + 12.0f;

	for (int32 i = Editor.EditSequence->Notifies.Num() - 1; i >= 0; --i)
	{
		const FAnimNotifyEvent& Notify = Editor.EditSequence->Notifies[i];
		if (!std::isfinite(Notify.TriggerTime) || !std::isfinite(Notify.Duration) ||
			Notify.TriggerTime < 0.0f || Notify.TriggerTime > Length || Notify.Duration < 0.0f) continue;

		const float StartX = Editor.TimelineView.TimeToX(Notify.TriggerTime, LaneMin.x);
		const float EndTime = static_cast<float>(std::min(static_cast<double>(Length), static_cast<double>(Notify.TriggerTime) + Notify.Duration));
		const float EndX = Editor.TimelineView.TimeToX(EndTime, LaneMin.x);

		const float HitLeft = std::max(LaneMin.x, StartX - 7.0f);
		const float HitRight = std::min(LaneMax.x, std::max(EndX + 4.0f, StartX + 7.0f));
		const float HitTop = CenterY - 9.0f;
		const float HitBottom = CenterY + 9.0f;

		if (MouseX >= HitLeft && MouseX <= HitRight && MouseY >= HitTop && MouseY <= HitBottom)
		{
			if (OutPart)
			{
				const bool bOnRightGrip = (Notify.Duration > 0.0f) && (EndX - StartX >= 8.0f) && (std::fabs(MouseX - EndX) <= 6.0f);
				*OutPart = bOnRightGrip ? ENotifyDragMode::ResizeEnd : ENotifyDragMode::Move;
			}
			return i;
		}
	}
	return INDEX_NONE;
}
