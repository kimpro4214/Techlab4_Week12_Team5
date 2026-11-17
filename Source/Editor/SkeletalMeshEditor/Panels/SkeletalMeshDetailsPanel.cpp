#include "EnginePCH.h"
#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshDetailsPanel.h"
#include "Editor/SkeletalMeshEditor/SkeletalMeshEditor.h"

namespace
{
	constexpr ImU32 AxisColors[3] = {
		IM_COL32(165, 1, 10, 255),
		IM_COL32(13, 90, 6, 255),
		IM_COL32(3, 34, 234, 255)
	};

	bool DrawResetButton(const char* StrId)
	{
		const float Size = ImGui::GetFrameHeight();
		const bool bPressed = ImGui::InvisibleButton(StrId, ImVec2(Size, Size));

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const ImVec2 Min = ImGui::GetItemRectMin();
		const ImVec2 Max = ImGui::GetItemRectMax();
		const ImVec2 Center(Min.x + Size * 0.5f, Min.y + Size * 0.5f);

		if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		{
			const ImGuiCol BgColor = ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered;
			DrawList->AddRectFilled(Min, Max, ImGui::GetColorU32(BgColor), ImGui::GetStyle().FrameRounding);
		}

		const ImU32 Color = ImGui::GetColorU32(ImGuiCol_Text);
		const float Radius = Size * 0.28f;
		constexpr float Thickness = 1.5f;

		constexpr float AngleStart = -IM_PI * 0.5f;
		constexpr float AngleEnd = IM_PI;
		DrawList->PathArcTo(Center, Radius, AngleStart, AngleEnd);
		DrawList->PathStroke(Color, ImDrawFlags_None, Thickness);

		const ImVec2 Radial(cosf(AngleStart), sinf(AngleStart));
		const ImVec2 Tangent(sinf(AngleStart), -cosf(AngleStart));
		const ImVec2 P(Center.x + Radial.x * Radius, Center.y + Radial.y * Radius);
		const float HalfWidth = Size * 0.16f;
		const float Length = Size * 0.24f;

		DrawList->AddTriangleFilled(
			ImVec2(P.x + Radial.x * HalfWidth, P.y + Radial.y * HalfWidth),
			ImVec2(P.x - Radial.x * HalfWidth, P.y - Radial.y * HalfWidth),
			ImVec2(P.x + Tangent.x * Length, P.y + Tangent.y * Length),
			Color);

		return bPressed;
	}

	bool DrawVector3Controller(const FString& Label, float* Values, const float* ResetValue, float LabelWidth)
	{
		ImGuiTableFlags Flags = ImGuiTableFlags_BordersInnerV;

		bool bVectorChanged = false;

		ImGui::PushID(Label.c_str());
		if (ImGui::BeginTable(Label.c_str(), 3, Flags))
		{
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, LabelWidth);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, 35.0f);

			ImGui::TableNextColumn();
			ImGui::Text("%s", Label.c_str());

			ImGui::TableNextColumn();
			ImGui::PushMultiItemsWidths(3, ImGui::GetContentRegionAvail().x);

			bool bValueChanged = false;
			for (int i = 0; i < 3; i++)
			{
				bValueChanged |= Values[i] != ResetValue[i];

				ImGui::PushID(i);

				if (i > 0)
				{
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
				}

				if (ImGui::DragFloat("##v", &Values[i], 0.1f, 0.0f, 0.0f, "%.2f"))
				{
					bVectorChanged = true;
				}

				const ImVec2 Min = ImGui::GetItemRectMin();
				const ImVec2 Max = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddRectFilled(Min, ImVec2(Min.x + 2.0f, Max.y), AxisColors[i]);

				ImGui::PopID();
				ImGui::PopItemWidth();
			}

			ImGui::TableNextColumn();
			if (bValueChanged)
			{
				if (DrawResetButton("Reset"))
				{
					for (int i = 0; i < 3; i++)
					{
						Values[i] = ResetValue[i];
					}
					bVectorChanged = true;
				}
			}

			ImGui::EndTable();
		}
		ImGui::PopID();

		return bVectorChanged;
	}

	bool DrawRotatorController(const FString& Label, FRotator& Rotator, const FRotator& ResetValue, float LabelWidth)
	{
		float Values[3] = { Rotator.Roll, Rotator.Pitch, Rotator.Yaw };
		const float ResetValues[3] = { ResetValue.Roll, ResetValue.Pitch, ResetValue.Yaw };

		const bool bChanged = DrawVector3Controller(Label, Values, ResetValues, LabelWidth);
		if (bChanged)
		{
			Rotator.Roll = Values[0];
			Rotator.Pitch = Values[1];
			Rotator.Yaw = Values[2];
		}
		return bChanged;
	}
}

void FSkeletalMeshDetailsPanel::DrawContent()
{
	const int32 BoneIndex = Editor.GetSelectedBoneIndex();
	const auto& Skeleton = Editor.GetReferenceSkeleton();
	if (!Skeleton.RawRefBoneInfo.IsValidIndex(BoneIndex))
	{
		ImGui::TextDisabled("Select a bone to edit its transform.");
		return;
	}
	ImGui::Text("Name: %s", Skeleton.RawRefBoneInfo[BoneIndex].Name.ToString().c_str());
	FTransform Transform = Editor.GetPreviewPose().LocalPose[BoneIndex];
	const FTransform& Reset = Skeleton.RawRefBonePose[BoneIndex];
	bool bChanged = DrawVector3Controller("Location", &Transform.Location[0], &Reset.Location.X, 90.0f);
	bChanged |= DrawRotatorController("Rotation", Transform.Rotation, Reset.Rotation, 90.0f);
	bChanged |= DrawVector3Controller("Scale", &Transform.Scale[0], &Reset.Scale.X, 90.0f);
	if (bChanged) Editor.SetBoneLocalTransform(BoneIndex, Transform);
}
