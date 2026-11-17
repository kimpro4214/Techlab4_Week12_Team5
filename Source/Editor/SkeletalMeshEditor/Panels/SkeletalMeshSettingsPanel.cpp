#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshSettingsPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "Components/SkeletalMeshComponent.h"

void FSkeletalMeshSettingsPanel::DrawContent()
{
	bool bGrid = Editor.IsRenderGrid();
	if (ImGui::Checkbox("Grid", &bGrid)) Editor.SetRenderGrid(bGrid);
	bool bMesh = Editor.IsRenderMesh();
	if (ImGui::Checkbox("Mesh", &bMesh)) Editor.SetRenderMesh(bMesh);
	bool bBones = Editor.IsBonesVisible();
	if (ImGui::Checkbox("Bones", &bBones)) Editor.SetRenderBones(bBones);

	const bool bCanShowWeight = USkeletalMeshComponent::IsGPUSkinningEnabled() && Editor.GetMesh();
	ImGui::BeginDisabled(!bCanShowWeight);
	bool bWeight = Editor.IsBoneWeightRequested();
	if (ImGui::Checkbox("Bone Weight", &bWeight)) Editor.SetBoneWeightEnabled(bWeight);
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip(bCanShowWeight ? "Show the selected bone's influence (0 to 1)." : "Requires GPU skinning.");
}
