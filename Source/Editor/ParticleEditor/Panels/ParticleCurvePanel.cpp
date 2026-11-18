#include "EnginePCH.h"
#include "ParticleCurvePanel.h"
#include "Curves/CurveFloat.h"
#include "Editor/ParticleEditor/ParticleEditor.h"
#include "Render/Particle/ParticleModuleColorOverLife.h"
#include <algorithm>
#include <cmath>

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
	ImGui::TextUnformatted("Color Blend Over Life");
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
		ViewZoom = 1.0f;
	}
	if (Curve)
	{
		if (ImGui::Button("Fit View")) ViewZoom = 1.0f;
		ImGui::SameLine();
		ImGui::TextDisabled("%.1fx", ViewZoom);
		ImGui::TextWrapped("Scroll to zoom. Double-click to add a key; right-click a key for options.");
		DrawGraph(*Curve);
		DrawKeyProperties(*Curve);
	}
	ImGui::PopID();
}

void FParticleCurvePanel::DrawGraph(UCurveFloat& Curve)
{
	constexpr int32 SampleCount = 128;
	const ImVec2 Size = ImGui::GetContentRegionAvail();
	const float GraphHeight = std::max(76.0f, Size.y - 4.0f * ImGui::GetFrameHeightWithSpacing());
	if (Size.x <= 20.0f) return;

	ImGui::InvisibleButton("##CurveGraph", ImVec2(Size.x, GraphHeight));
	const bool bActive = ImGui::IsItemActive();
	const bool bHovered = ImGui::IsItemHovered();
	const bool bDraggingKey = bActive && DraggedKeyId >= 0 && !ImGui::IsItemActivated();
	if (!bDraggingKey) DraggedKeyId = -1;

	const ImVec2 BoxMin = ImGui::GetItemRectMin();
	const ImVec2 BoxMax = ImGui::GetItemRectMax();
	const ImVec2 PlotMin(BoxMin.x + 8.0f, BoxMin.y + 8.0f);
	const ImVec2 PlotMax(BoxMax.x - 8.0f, BoxMax.y - 8.0f);
	const float Width = PlotMax.x - PlotMin.x;
	const float Height = PlotMax.y - PlotMin.y;
	const ImVec2 Mouse = ImGui::GetMousePos();
	const float Wheel = ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY) && bHovered && !bActive
		? ImGui::GetIO().MouseWheel : 0.0f;
	const float NewZoom = std::clamp(ViewZoom * std::pow(1.25f, Wheel), 1.0f, 32.0f);

	// 전체 보기에서는 키와 보간 결과가 화면에 들어오도록 높이를 맞춥니다.
	float FitMinValue = 0.0f;
	float FitMaxValue = 1.0f;
	if ((ViewZoom == 1.0f && !bDraggingKey) || (ViewZoom != 1.0f && NewZoom == 1.0f))
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
		ViewMinValue = bDraggingKey ? DragMinValue : FitMinValue;
		ViewMaxValue = bDraggingKey ? DragMaxValue : FitMaxValue;
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
	DrawList->PushClipRect(BoxMin, BoxMax, true);
	for (int32 Index = 0; Index <= 4; ++Index)
	{
		const float Fraction = Index / 4.0f;
		const float X = PlotMin.x + Fraction * Width;
		const float Y = PlotMin.y + Fraction * Height;
		const ImU32 GridColor = ImGui::GetColorU32(ImGuiCol_Border);
		DrawList->AddLine(ImVec2(X, PlotMin.y), ImVec2(X, PlotMax.y), GridColor);
		DrawList->AddLine(ImVec2(PlotMin.x, Y), ImVec2(PlotMax.x, Y), GridColor);
	}
	ImVec2 Previous = ToScreen(ViewMinTime, Curve.GetFloatValue(ViewMinTime));
	for (int32 Index = 1; Index <= SampleCount; ++Index)
	{
		const float Time = ViewMinTime + TimeSpan * Index / SampleCount;
		const ImVec2 Point = ToScreen(Time, Curve.GetFloatValue(Time));
		DrawList->AddLine(Previous, Point, ImGui::GetColorU32(ImGuiCol_PlotLines), 2.0f);
		Previous = Point;
	}

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
	DrawList->PopClipRect();

	const float Time = std::clamp(ViewMinTime + (Mouse.x - PlotMin.x) / Width * TimeSpan, 0.0f, 1.0f);
	const float Value = ViewMinValue + (PlotMax.y - Mouse.y) / Height * ValueSpan;
	if (ImGui::IsItemActivated())
	{
		SelectedKeyId = HoveredKeyId;
		DraggedKeyId = HoveredKeyId;
		DragMinValue = ViewMinValue;
		DragMaxValue = ViewMaxValue;
		if (HoveredKeyId < 0 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
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
	if (bDraggingKey && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		Curve.SetKeyTime(DraggedKeyId, Time);
		Curve.SetKeyValue(DraggedKeyId, Value);
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
			SelectedKeyId = -1;
			DraggedKeyId = -1;
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void FParticleCurvePanel::DrawKeyProperties(UCurveFloat& Curve)
{
	ImGui::TextDisabled("X: Normalized Lifetime (0-1)    Y: Blend");
	const FFloatCurveKey* Key = Curve.FindKey(SelectedKeyId);
	if (!Key) return;

	// 시각을 바꾸면 키 배열이 재정렬되므로 편집할 값을 먼저 복사합니다.
	FFloatCurveKey Edited = *Key;
	ImGui::PushID(Edited.KeyId);
	if (ImGui::DragFloat("Time", &Edited.Time, 0.005f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
	{
		Curve.SetKeyTime(Edited.KeyId, Edited.Time);
	}
	if (ImGui::DragFloat("Value", &Edited.Value, 0.005f))
	{
		Curve.SetKeyValue(Edited.KeyId, Edited.Value);
	}

	const char* Modes[] = { "Constant", "Linear", "Cubic", "Spline" };
	int Mode = static_cast<int>(Edited.InterpMode);
	if (ImGui::Combo("Interpolation", &Mode, Modes, IM_ARRAYSIZE(Modes)))
	{
		Edited.InterpMode = static_cast<ECurveInterpMode>(Mode);
		Curve.SetKeyInterpMode(Edited.KeyId, Edited.InterpMode);
	}
	if (Edited.InterpMode == ECurveInterpMode::Cubic)
	{
		bool bChanged = ImGui::DragFloat("Arrive Tangent", &Edited.ArriveTangent, 0.05f);
		bChanged |= ImGui::DragFloat("Leave Tangent", &Edited.LeaveTangent, 0.05f);
		if (bChanged) Curve.SetKeyTangents(Edited.KeyId, Edited.ArriveTangent, Edited.LeaveTangent);
	}
	if (ImGui::Button("Delete Key"))
	{
		Curve.RemoveKey(Edited.KeyId);
		SelectedKeyId = -1;
	}
	ImGui::PopID();
}
