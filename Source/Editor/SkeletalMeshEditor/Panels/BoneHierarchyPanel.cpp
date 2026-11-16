#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/Panels/BoneHierarchyPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"

void FBoneHierarchyPanel::DrawContent()
{
	if (ImGui::Button("Reset All")) Editor.ResetPose();
	const auto& Bones = Editor.GetReferenceSkeleton().RawRefBoneInfo;
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		if (Bones[Index].ParentIndex < 0) DrawBoneTreeNode(Index);
	}
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		Editor.SelectBone(-1);
}

void FBoneHierarchyPanel::DrawBoneTreeNode(int32 BoneIndex)
{
	const auto& Skeleton = Editor.GetReferenceSkeleton();
	const auto& Children = Editor.GetBoneChildren();
	const bool bLeaf = Children[BoneIndex].IsEmpty();
	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
	if (bLeaf) Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (BoneIndex == Editor.GetSelectedBoneIndex()) Flags |= ImGuiTreeNodeFlags_Selected;
	const bool bChanged = Skeleton.RawRefBonePose[BoneIndex] != Editor.GetPreviewPose().LocalPose[BoneIndex];
	const FString Name = Skeleton.RawRefBoneInfo[BoneIndex].Name.ToString() + (bChanged ? " *" : "");
	ImGui::PushID(BoneIndex);
	const bool bOpen = ImGui::TreeNodeEx("##Bone", Flags, "%s", Name.c_str());
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) Editor.SelectBone(BoneIndex);
	if (bOpen && !bLeaf)
	{
		for (int32 Child : Children[BoneIndex]) DrawBoneTreeNode(Child);
		ImGui::TreePop();
	}
	ImGui::PopID();
}
