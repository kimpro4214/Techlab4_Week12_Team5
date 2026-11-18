#pragma once
#include "Editor/EditorUI/EditorPanel.h"
#include "Editor/AnimationSequenceEditor/AnimationSequenceEditor.h"

class FAnimationSequenceEditorPanel : public IEditorPanel
{
public:
	enum class ERole { Viewport, Hierarchy, SequenceInfo, Timeline, NotifyDetails };
	FAnimationSequenceEditorPanel(FAnimationSequenceEditor& InEditor, ERole InRole, const char* InName);
	bool Init() override { return true; }
	void Tick(float) override {}
	void OnRender() override;
	const char* GetPanelName() const override { return PanelName.c_str(); }
	const char* GetWindowName() const { return WindowName.c_str(); }
	void SetDockClass(ImGuiID InId) { DockClassId = InId; }
	bool IsDetached(ImGuiID DockSpaceId) const;
private:
	using ENotifyDragMode = FAnimationSequenceEditor::ENotifyDragMode;
	void DrawViewport();
	void TickCameraInput();
	void DrawBoneHierarchy();
	void DrawBoneTreeNode(int32 BoneIndex);
	void DrawSequenceInfo();
	void DrawSelectedNotifyInspector();
	void DrawPlaybackControls();
	void DrawAnimationTimeline();
	void DrawNotifyTrack(ImDrawList* Draw, const ImVec2& Min, const ImVec2& Max);
	int32 HitTestNotify(float MouseX, float MouseY, const ImVec2& LaneMin, const ImVec2& LaneMax, ENotifyDragMode* OutPart = nullptr) const;
	FAnimationSequenceEditor& Editor;
	ERole Role;
	FString PanelName;
	FString WindowName;
	ImGuiID DockClassId = 0;
};
