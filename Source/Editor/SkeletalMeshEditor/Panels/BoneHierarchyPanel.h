#pragma once

#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshEditorPanel.h"

class FBoneHierarchyPanel : public FSkeletalMeshEditorPanel
{
public:
	explicit FBoneHierarchyPanel(FSkeletalMeshEditor& InEditor) : FSkeletalMeshEditorPanel(InEditor, "Bone Hierarchy") {}

private:
	void DrawContent() override;
	void DrawBoneTreeNode(int32 BoneIndex);
};
