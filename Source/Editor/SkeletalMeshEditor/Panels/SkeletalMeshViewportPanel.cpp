#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshViewportPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"

void FSkeletalMeshViewportPanel::DrawContent()
{
	FGizmo& Gizmo = Editor.GetBoneGizmo();
	if (ImGui::RadioButton("Location", Gizmo.GetMode() == EGizmoMode::Location)) Gizmo.SetMode(EGizmoMode::Location);
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotation", Gizmo.GetMode() == EGizmoMode::Rotation)) Gizmo.SetMode(EGizmoMode::Rotation);
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", Gizmo.GetMode() == EGizmoMode::Scale)) Gizmo.SetMode(EGizmoMode::Scale);
	ImGui::SameLine();
	const bool bLocal = Gizmo.GetSpace() == EGizmoSpace::Local;
	if (ImGui::Button(bLocal ? "Local" : "World")) Gizmo.SetSpace(bLocal ? EGizmoSpace::World : EGizmoSpace::Local);

	if (Editor.IsBoneWeightEnabled())
	{
		const int32 BoneIndex = Editor.GetBoneWeightBoneIndex();
		if (BoneIndex >= 0)
			ImGui::Text("Bone Weight: %s | 0 Purple > Blue > Green > Yellow > Red 1",
				Editor.GetReferenceSkeleton().RawRefBoneInfo[BoneIndex].Name.ToString().c_str());
		else ImGui::TextDisabled("No bone selected: Weight 0 (Purple).");
	}

	const ImVec2 Available = ImGui::GetContentRegionAvail();
	if (Available.x <= 0.0f || Available.y <= 0.0f) return;
	FEditorViewportClient* Client = Editor.GetViewportClient();
	Client->Resize(static_cast<uint32>(std::max(1.0f, Available.x)), static_cast<uint32>(std::max(1.0f, Available.y)));
	if (!Client->GetColorTarget() || !Client->GetColorTarget()->GetSRV()) return;
	Editor.SetViewportVisible(true);

	const ImVec2 Min = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##MeshViewport", Available, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	ImGui::GetWindowDrawList()->AddImage(Client->GetColorTarget()->GetSRV(), Min,
		ImVec2(Min.x + Available.x, Min.y + Available.y));
	if (ImGui::IsItemActivated()) ImGui::SetWindowFocus();

	const ImGuiIO& IO = ImGui::GetIO();
	const bool bCaptured = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Right) && !Gizmo.IsUsing();
	FSkeletalMeshViewportInput Input;
	Input.Camera = {
		.bCaptured = bCaptured,
		.bAllowWheel = bCaptured || ImGui::IsItemHovered(),
		.bForward = ImGui::IsKeyDown(ImGuiKey_W),
		.bBackward = ImGui::IsKeyDown(ImGuiKey_S),
		.bRight = ImGui::IsKeyDown(ImGuiKey_D),
		.bLeft = ImGui::IsKeyDown(ImGuiKey_A),
		.bUp = ImGui::IsKeyDown(ImGuiKey_E),
		.bDown = ImGui::IsKeyDown(ImGuiKey_Q),
		.MouseX = IO.MouseDelta.x,
		.MouseY = IO.MouseDelta.y,
		.Wheel = IO.MouseWheel * 120.0f
	};
	Input.LocalMouse = FVector2(IO.MousePos.x - Min.x, IO.MousePos.y - Min.y);
	Input.bHovered = ImGui::IsItemHovered();
	Input.bLeftPressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	Input.bLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	Input.bItemActive = ImGui::IsItemActive();
	Editor.ProcessViewportInput(Input, IO.DeltaTime);
}
