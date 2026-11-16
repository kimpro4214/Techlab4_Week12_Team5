#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshInfoPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"
#include "Render/SkeletalMesh.h"

void FSkeletalMeshInfoPanel::DrawContent()
{
	ImGui::TextWrapped("Path: %s", Editor.GetKey().AssetPath.c_str());
	ImGui::Text("Bones: %d", Editor.GetReferenceSkeleton().RawRefBoneInfo.Num());
	ImGui::Text("Vertices: %d", Editor.GetMesh() ? Editor.GetMesh()->GetMeshData().ReferenceVertices.Num() : 0);
	if (Editor.IsDirty()) ImGui::TextDisabled("Unsaved pose changes");
}
