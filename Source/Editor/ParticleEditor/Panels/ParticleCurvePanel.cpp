#include "EnginePCH.h"
#include "ParticleCurvePanel.h"
#include "Curves/CurveFloat.h"
#include "Editor/ParticleEditor/ParticleEditor.h"
#include "Render/Particle/ParticleModuleColorOverLife.h"
#include <algorithm>
#include <cmath>

namespace
{
bool HasCubicTangent(const UCurveFloat& Curve, const FFloatCurveKey& Key, bool bArrive)
{
	const auto& Keys = Curve.GetKeys();
	const auto Index = &Key - &Keys[0];
	// 도착 접선은 이전 키에서 시작한 구간에 사용됩니다.
	if (bArrive) return Index > 0 && Keys[Index - 1].InterpMode == ECurveInterpMode::Cubic;
	return Index + 1 < Keys.Num() && Key.InterpMode == ECurveInterpMode::Cubic;
}
}

FParticleCurvePanel::FParticleCurvePanel(FParticleEditor& InEditor)
	: FParticleEditorPanel(InEditor, "Curve Editor")
{
}

void FParticleCurvePanel::DrawContent()
{
	auto* Module = Cast<UParticleModuleColorOverLife>(Editor.GetSelectedModule());
	if (!Module)
	{
		SelectedCurve = nullptr;
		ImGui::TextWrapped("Select a Color Over Life module in Emitters.");
		return;
	}

	ImGui::PushID(Module);
	if (!Module->ColorBlendCurve && ImGui::Button("Create Curve"))
	{
		// 커브가 없을 때의 선형 색상 전환을 초기 키로 유지합니다.
		Module->ColorBlendCurve = NewObject<UCurveFloat>(Module);
		Module->ColorBlendCurve->AddKey(0.0f, 0.0f);
		Module->ColorBlendCurve->AddKey(1.0f, 1.0f);
		Editor.RefreshSystem();
	}

	UCurveFloat* Curve = Module->ColorBlendCurve;
	if (SelectedCurve != Curve)
	{
		SelectedCurve = Curve;
		SelectedKeyId = -1;
		DraggedKeyId = -1;
		DraggedTangent = ETangentHandle::None;
		TangentHandleLengths.clear();
		ViewZoom = 1.0f;
	}
	if (Curve)
	{
		DrawGraph(*Curve);
		DrawKeyProperties(*Curve);
	}
	ImGui::PopID();
}

void FParticleCurvePanel::DrawGraph(UCurveFloat& Curve)
{
	constexpr int32 SampleCount = 128;
	constexpr float DefaultHandleLength = 32.0f;
	constexpr float PlotPadding = DefaultHandleLength + 5.0f;
	const ImVec2 Size = ImGui::GetContentRegionAvail();
	const float PropertiesHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ScrollbarSize;
	const float GraphHeight = std::max(100.0f, Size.y - PropertiesHeight - ImGui::GetStyle().ItemSpacing.y);
	if (Size.x <= 2.0f * PlotPadding) return;

	ImGui::InvisibleButton("##CurveGraph", ImVec2(Size.x, GraphHeight));
	const bool bActive = ImGui::IsItemActive();
	const bool bHovered = ImGui::IsItemHovered();
	const bool bDragging = bActive && DraggedKeyId >= 0 && !ImGui::IsItemActivated();
	if (!bDragging)
	{
		DraggedKeyId = -1;
		DraggedTangent = ETangentHandle::None;
	}

	const ImVec2 BoxMin = ImGui::GetItemRectMin();
	const ImVec2 BoxMax = ImGui::GetItemRectMax();
	const ImVec2 PlotMin(BoxMin.x + PlotPadding, BoxMin.y + PlotPadding);
	const ImVec2 PlotMax(BoxMax.x - PlotPadding, BoxMax.y - PlotPadding);
	const float Width = PlotMax.x - PlotMin.x;
	const float Height = PlotMax.y - PlotMin.y;
	const ImVec2 Mouse = ImGui::GetMousePos();
	const float Wheel = ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY) && bHovered && !bActive
		? ImGui::GetIO().MouseWheel : 0.0f;
	const float NewZoom = std::clamp(ViewZoom * std::pow(1.25f, Wheel), 1.0f, 32.0f);

	// 전체 보기에서는 키와 보간 결과가 화면에 들어오도록 높이를 맞춥니다.
	float FitMinValue = 0.0f;
	float FitMaxValue = 1.0f;
	if ((ViewZoom == 1.0f && !bDragging) || (ViewZoom != 1.0f && NewZoom == 1.0f))
	{
		for (int32 Index = 0; Index <= SampleCount; ++Index)
		{
			const float Value = Curve.GetFloatValue(static_cast<float>(Index) / SampleCount);
			FitMinValue = std::min(FitMinValue, Value);
			FitMaxValue = std::max(FitMaxValue, Value);
		}
		for (const FFloatCurveKey& Key : Curve.GetKeys())
		{
			if (Key.Time < 0.0f || Key.Time > 1.0f) continue;
			FitMinValue = std::min(FitMinValue, Key.Value);
			FitMaxValue = std::max(FitMaxValue, Key.Value);
		}
	}

	if (ViewZoom == 1.0f)
	{
		ViewMinTime = 0.0f;
		ViewMinValue = bDragging ? DragMinValue : FitMinValue;
		ViewMaxValue = bDragging ? DragMaxValue : FitMaxValue;
		GridMinValue = ViewMinValue;
		GridMaxValue = ViewMaxValue;
	}

	if (NewZoom != ViewZoom)
	{
		const float AnchorX = std::clamp((Mouse.x - PlotMin.x) / Width, 0.0f, 1.0f);
		const float AnchorY = std::clamp((PlotMax.y - Mouse.y) / Height, 0.0f, 1.0f);
		const float OldSpanY = ViewMaxValue - ViewMinValue;
		const float NewSpanY = OldSpanY * (ViewZoom / NewZoom);

		ViewMinTime = std::clamp(ViewMinTime + AnchorX * (1.0f / ViewZoom - 1.0f / NewZoom),
			0.0f, 1.0f - 1.0f / NewZoom);
		ViewMinValue += AnchorY * (OldSpanY - NewSpanY);
		ViewMaxValue = ViewMinValue + NewSpanY;
		ViewZoom = NewZoom;
		if (ViewZoom == 1.0f)
		{
			ViewMinValue = FitMinValue;
			ViewMaxValue = FitMaxValue;
			GridMinValue = FitMinValue;
			GridMaxValue = FitMaxValue;
		}
	}

	const float TimeSpan = 1.0f / ViewZoom;
	const float ValueSpan = ViewMaxValue - ViewMinValue;
	const auto ToScreen = [&](float Time, float Value)
	{
		return ImVec2(PlotMin.x + (Time - ViewMinTime) / TimeSpan * Width,
			PlotMax.y - (Value - ViewMinValue) / ValueSpan * Height);
	};

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	DrawList->AddRectFilled(BoxMin, BoxMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
	DrawList->PushClipRect(PlotMin, PlotMax, true);

	// 시간 좌표에 눈금을 고정해 확대 시 곡선과 같은 비율로 움직이게 합니다.
	constexpr int32 MajorDivisions = 10;
	constexpr int32 Subdivisions = 10;
	constexpr int32 TotalDivisions = MajorDivisions * Subdivisions;
	const ImU32 MajorColor = ImGui::GetColorU32(ImGuiCol_Border);
	const ImU32 MinorColor = ImGui::GetColorU32(ImGuiCol_Border, 0.35f);
	for (int32 Index = 0; Index <= TotalDivisions; ++Index)
	{
		const float Time = static_cast<float>(Index) / TotalDivisions;
		if (Time < ViewMinTime || Time > ViewMinTime + TimeSpan) continue;
		const float X = ToScreen(Time, ViewMinValue).x;
		DrawList->AddLine(ImVec2(X, PlotMin.y), ImVec2(X, PlotMax.y), Index % Subdivisions == 0 ? MajorColor : MinorColor);
	}
	for (int32 Index = 0; Index <= 4; ++Index)
	{
		const float TickValue = std::lerp(GridMinValue, GridMaxValue, Index / 4.0f);
		if (TickValue < ViewMinValue || TickValue > ViewMaxValue) continue;
		const float Y = ToScreen(ViewMinTime, TickValue).y;
		DrawList->AddLine(ImVec2(PlotMin.x, Y), ImVec2(PlotMax.x, Y), MajorColor);
	}
	ImVec2 Previous = ToScreen(ViewMinTime, Curve.GetFloatValue(ViewMinTime));
	for (int32 Index = 1; Index <= SampleCount; ++Index)
	{
		const float Time = ViewMinTime + TimeSpan * Index / SampleCount;
		const ImVec2 Point = ToScreen(Time, Curve.GetFloatValue(Time));
		DrawList->AddLine(Previous, Point, ImGui::GetColorU32(ImGuiCol_PlotLines), 2.0f);
		Previous = Point;
	}
	DrawList->PopClipRect();
	DrawList->PushClipRect(BoxMin, BoxMax, true);

	float ClosestDistanceSquared = 64.0f;
	int32 HoveredKeyId = -1;
	for (const FFloatCurveKey& Key : Curve.GetKeys())
	{
		if (Key.Time < ViewMinTime || Key.Time > ViewMinTime + TimeSpan ||
			Key.Value < ViewMinValue || Key.Value > ViewMaxValue) continue;
		const ImVec2 Point = ToScreen(Key.Time, Key.Value);
		const float DX = Point.x - Mouse.x;
		const float DY = Point.y - Mouse.y;
		const float DistanceSquared = DX * DX + DY * DY;
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			HoveredKeyId = Key.KeyId;
		}

		const ImGuiCol Color = Key.KeyId == SelectedKeyId ? ImGuiCol_PlotLinesHovered : ImGuiCol_Text;
		DrawList->AddCircleFilled(Point, 4.0f, ImGui::GetColorU32(Color));
	}

	ETangentHandle HoveredTangent = ETangentHandle::None;
	const FFloatCurveKey* SelectedKey = Curve.FindKey(SelectedKeyId);
	const float SlopeToScreen = (Height / Width) * (TimeSpan / ValueSpan);
	if (SelectedKey && SelectedKey->Time >= ViewMinTime && SelectedKey->Time <= ViewMinTime + TimeSpan &&
		SelectedKey->Value >= ViewMinValue && SelectedKey->Value <= ViewMaxValue)
	{
		const ImVec2 KeyPoint = ToScreen(SelectedKey->Time, SelectedKey->Value);
		const auto LengthIt = TangentHandleLengths.find(SelectedKeyId);
		const float HandleLength = LengthIt == TangentHandleLengths.end() ? DefaultHandleLength : LengthIt->second;
		for (ETangentHandle Side : { ETangentHandle::Arrive, ETangentHandle::Leave })
		{
			const bool bArrive = Side == ETangentHandle::Arrive;
			if (!HasCubicTangent(Curve, *SelectedKey, bArrive)) continue;

			const float Slope = bArrive ? SelectedKey->ArriveTangent : SelectedKey->LeaveTangent;
			const float Angle = std::atan(-Slope * SlopeToScreen);
			const float Length = bArrive ? -HandleLength : HandleLength;
			const ImVec2 HandlePoint(KeyPoint.x + std::cos(Angle) * Length, KeyPoint.y + std::sin(Angle) * Length);
			const float DX = HandlePoint.x - Mouse.x;
			const float DY = HandlePoint.y - Mouse.y;
			if (DX * DX + DY * DY <= 36.0f) HoveredTangent = Side;

			const ImGuiCol Color = HoveredTangent == Side || DraggedTangent == Side
				? ImGuiCol_PlotLinesHovered : ImGuiCol_SliderGrab;
			const ImU32 HandleColor = ImGui::GetColorU32(Color);
			DrawList->AddLine(KeyPoint, HandlePoint, HandleColor);
			DrawList->AddCircleFilled(HandlePoint, 4.5f, HandleColor);
		}
	}
	DrawList->PopClipRect();

	const float Time = std::clamp(ViewMinTime + (Mouse.x - PlotMin.x) / Width * TimeSpan, 0.0f, 1.0f);
	const float Value = ViewMinValue + (PlotMax.y - Mouse.y) / Height * ValueSpan;
	if (ImGui::IsItemActivated())
	{
		DraggedTangent = HoveredTangent;
		if (DraggedTangent == ETangentHandle::None) SelectedKeyId = HoveredKeyId;
		DraggedKeyId = SelectedKeyId;
		DragMinValue = ViewMinValue;
		DragMaxValue = ViewMaxValue;
		if (DraggedKeyId < 0 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			// 같은 시각을 클릭해도 기존 키의 값을 덮어쓰지 않습니다.
			for (const FFloatCurveKey& Key : Curve.GetKeys())
			{
				if (Key.Time == Time)
				{
					SelectedKeyId = Key.KeyId;
					return;
				}
			}
			SelectedKeyId = Curve.AddKey(Time, Value);
		}
	}
	if (bDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		if (DraggedTangent == ETangentHandle::None)
		{
			Curve.SetKeyTime(DraggedKeyId, Time);
			Curve.SetKeyValue(DraggedKeyId, Value);
		}
		else if (SelectedKey)
		{
			const ImVec2 KeyPoint = ToScreen(SelectedKey->Time, SelectedKey->Value);
			const bool bArrive = DraggedTangent == ETangentHandle::Arrive;
			const float DX = std::clamp(Mouse.x, BoxMin.x + 5.0f, BoxMax.x - 5.0f) - KeyPoint.x;
			const float DY = KeyPoint.y - std::clamp(Mouse.y, BoxMin.y + 5.0f, BoxMax.y - 5.0f);
			// 접선 핸들은 해당 구간의 방향을 유지하며 기울기만 바꿉니다.
			const float TangentDX = bArrive ? std::min(-0.5f, DX) : std::max(0.5f, DX);
			const float Slope = DY / TangentDX / SlopeToScreen;
			Curve.SetKeyTangents(DraggedKeyId, Slope, Slope);
			TangentHandleLengths[DraggedKeyId] = std::max(10.0f, std::hypot(TangentDX, DY));
		}
	}

	ImGui::PushID(&Curve);
	if (bHovered && !bActive && HoveredKeyId >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		SelectedKeyId = HoveredKeyId;
		ImGui::OpenPopup("Key Options");
	}
	if (ImGui::BeginPopup("Key Options"))
	{
		if (ImGui::MenuItem("Delete Key"))
		{
			Curve.RemoveKey(SelectedKeyId);
			TangentHandleLengths.erase(SelectedKeyId);
			SelectedKeyId = -1;
			DraggedKeyId = -1;
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void FParticleCurvePanel::DrawKeyProperties(UCurveFloat& Curve)
{
	const FFloatCurveKey* Key = Curve.FindKey(SelectedKeyId);
	if (!Key) return;

	const float RowHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ScrollbarSize;
	const float NumberWidth = ImGui::GetFontSize() * 5.0f;
	ImGui::BeginChild("Key Properties", ImVec2(0.0f, RowHeight), ImGuiChildFlags_None,
		ImGuiWindowFlags_HorizontalScrollbar);

	// 시각을 바꾸면 키 배열이 재정렬되므로 편집할 값을 먼저 복사합니다.
	FFloatCurveKey Edited = *Key;
	ImGui::PushID(Edited.KeyId);
	ImGui::SetNextItemWidth(NumberWidth);
	if (ImGui::DragFloat("Time", &Edited.Time, 0.005f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
	{
		Curve.SetKeyTime(Edited.KeyId, Edited.Time);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(NumberWidth);
	if (ImGui::DragFloat("Value", &Edited.Value, 0.005f))
	{
		Curve.SetKeyValue(Edited.KeyId, Edited.Value);
	}

	const char* Modes[] = { "Constant", "Linear", "Cubic" };
	const int Mode = static_cast<int>(Edited.InterpMode);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::CalcTextSize("Constant").x + ImGui::GetFrameHeight() * 1.5f);
	if (ImGui::BeginCombo("Interpolation", Mode < IM_ARRAYSIZE(Modes) ? Modes[Mode] : "Legacy"))
	{
		for (int Index = 0; Index < IM_ARRAYSIZE(Modes); ++Index)
		{
			if (ImGui::Selectable(Modes[Index], Index == Mode))
			{
				Curve.SetKeyInterpMode(Edited.KeyId, static_cast<ECurveInterpMode>(Index));
			}
		}
		ImGui::EndCombo();
	}
	Key = Curve.FindKey(Edited.KeyId);
	bool bChanged = false;
	if (HasCubicTangent(Curve, *Key, true))
	{
		ImGui::SameLine();
		ImGui::SetNextItemWidth(NumberWidth);
		bChanged |= ImGui::DragFloat("Arrive Tangent", &Edited.ArriveTangent, 0.05f);
	}
	if (HasCubicTangent(Curve, *Key, false))
	{
		ImGui::SameLine();
		ImGui::SetNextItemWidth(NumberWidth);
		bChanged |= ImGui::DragFloat("Leave Tangent", &Edited.LeaveTangent, 0.05f);
	}
	if (bChanged) Curve.SetKeyTangents(Edited.KeyId, Edited.ArriveTangent, Edited.LeaveTangent);

	ImGui::SameLine();
	if (ImGui::Button("Delete Key"))
	{
		Curve.RemoveKey(Edited.KeyId);
		TangentHandleLengths.erase(Edited.KeyId);
		SelectedKeyId = -1;
	}
	ImGui::PopID();
	ImGui::EndChild();
}
