#include "EnginePCH.h"
#include "Editor/UIEditor/UIEditorPanel.h"
#include "Editor/Details/PropertyEditor.h"
#include "UI/UserWidget.h"
#include "UObject/UObjectHash.h"

FUIEditorPanel::FUIEditorPanel(FUIEditor& InEditor, ERole InRole, const char* InName)
	: Editor(InEditor), Role(InRole), PanelName(InName)
{
	WindowName = PanelName + "###UIPanel:" + PanelName + ":" + Editor.GetKey().GetTabId();
	PayloadType = std::format("UI_WIDGET:{:08x}", ImHashStr(Editor.GetKey().GetTabId().c_str()));
}

bool FUIEditorPanel::IsDetached(ImGuiID DockSpaceId) const
{
	const ImGuiWindow* Window = ImGui::FindWindowByName(GetWindowName());
	return Window && (!Window->DockNode || ImGui::DockNodeGetRootNode(Window->DockNode)->ID != DockSpaceId);
}

void FUIEditorPanel::OnRender()
{
	ImGuiWindowClass WindowClass;
	WindowClass.ClassId = DockClassId;
	WindowClass.DockingAllowUnclassed = false;
	ImGui::SetNextWindowClass(&WindowClass);
	bool bOpen = IsOpen();
	if (ImGui::Begin(GetWindowName(), &bOpen))
	{
		Editor.ReportPanelFocus(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
		switch (Role)
		{
		case ERole::Palette: DrawPalette(); break;
		case ERole::Hierarchy: DrawHierarchy(Editor.Root); break;
		case ERole::Canvas:
			Editor.SetCanvasVisible();
			DrawCanvas();
			break;
		case ERole::Details: DrawDetails(); break;
		}
	}
	ImGui::End();
	SetOpen(bOpen);
}

namespace
{
	bool IsSelfOrAncestor(const UWidget* Candidate, const UWidget* Widget)
	{
		for (const UWidget* It = Widget; It; It = It->GetParent())
			if (It == Candidate) return true;
		return false;
	}
}

void FUIEditorPanel::DrawCanvas()
{
	FTexture2D* Preview = Editor.PreviewTargets.SceneColor.get();
	if (!Preview || !Preview->GetSRV()) return;

	const ImVec2 Avail = ImGui::GetContentRegionAvail();
	if (Avail.x <= 0.0f || Avail.y <= 0.0f) return;
	ViewScale = std::min(Avail.x / Editor.PreviewSize.X, Avail.y / Editor.PreviewSize.Y);
	const ImVec2 Size(Editor.PreviewSize.X * ViewScale, Editor.PreviewSize.Y * ViewScale);

	CanvasMin = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##UICanvas", Size);
	const bool bHovered = ImGui::IsItemHovered();

	ImDrawList* DL = ImGui::GetWindowDrawList();
	const ImVec2 CanvasMax(CanvasMin.x + Size.x, CanvasMin.y + Size.y);
	DL->AddImage(Preview->GetSRV(), CanvasMin, CanvasMax);
	DL->AddRect(CanvasMin, CanvasMax, IM_COL32(90, 98, 110, 255));

	if (Editor.Selected && Editor.Selected != Editor.Root)
	{
		const ImU32 Accent = IM_COL32(255, 170, 40, 255);
		const FUIRect& R = Editor.Selected->GetCachedRect();
		DL->AddRect(ToScreen(R.X, R.Y), ToScreen(R.X + R.W, R.Y + R.H), Accent, 0.0f, 0, 2.0f);

		// 오른쪽 아래 크기 조절 손잡이
		const ImVec2 H = ToScreen(R.X + R.W, R.Y + R.H);
		DL->AddRectFilled(ImVec2(H.x - 5, H.y - 5), ImVec2(H.x + 5, H.y + 5), Accent);

		// 앵커: 부모 영역 안의 점(마름모) 또는 영역(점선 사각형)
		if (const UPanelWidget* Parent = Editor.Selected->GetParent())
		{
			const FUIRect& P = Parent->GetCachedRect();
			const FVector4& A = Editor.Selected->Anchors;
			const ImVec2 A0 = ToScreen(P.X + P.W * A.X, P.Y + P.H * A.Y);
			const ImVec2 A1 = ToScreen(P.X + P.W * A.Z, P.Y + P.H * A.W);
			if (A.X == A.Z && A.Y == A.W)
				DL->AddQuad(ImVec2(A0.x, A0.y - 7), ImVec2(A0.x + 7, A0.y), ImVec2(A0.x, A0.y + 7), ImVec2(A0.x - 7, A0.y), Accent, 2.0f);
			else
				DL->AddRect(A0, A1, IM_COL32(255, 170, 40, 140));
		}
	}

	HandleCanvasInput(bHovered);
}

bool FUIEditorPanel::IsOverResizeHandle(float X, float Y) const
{
	if (!Editor.Selected) return false;
	const FUIRect& R = Editor.Selected->GetCachedRect();
	const float Grab = 8.0f / ViewScale;   // 화면에서 8픽셀
	return std::abs(X - (R.X + R.W)) < Grab && std::abs(Y - (R.Y + R.H)) < Grab;
}

void FUIEditorPanel::HandleCanvasInput(bool bCanvasHovered)
{
	const ImVec2 Mouse = ImGui::GetMousePos();
	const float X = (Mouse.x - CanvasMin.x) / ViewScale;   // 디자인 좌표
	const float Y = (Mouse.y - CanvasMin.y) / ViewScale;

	if (bCanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		Editor.DragMode = FUIEditor::EDragMode::None;
		if (IsOverResizeHandle(X, Y))
			Editor.DragMode = FUIEditor::EDragMode::Resize;
		else if ((Editor.Selected = Editor.PickWidget(Editor.Root, X, Y)) != nullptr)
			Editor.DragMode = FUIEditor::EDragMode::Move;

		if (Editor.Selected)
		{
			Editor.DragStartX = X;
			Editor.DragStartY = Y;
			Editor.DragStartOffsets = Editor.Selected->Offsets;
			if (Editor.DragMode == FUIEditor::EDragMode::Resize && Editor.Selected->bAutoSize)
			{
				// 자동 크기였다면 지금 보이는 크기에서 시작한다.
				const FUIRect& R = Editor.Selected->GetCachedRect();
				Editor.DragStartOffsets.Z = R.W;
				Editor.DragStartOffsets.W = R.H;
				Editor.Selected->bAutoSize = false;
			}
		}
	}

	if (Editor.DragMode == FUIEditor::EDragMode::None || !Editor.Selected) return;
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		Editor.DragMode = FUIEditor::EDragMode::None;   // 놓는 순간 CommitUndoIfChanged가 스냅샷을 쌓는다
		return;
	}

	float DX = X - Editor.DragStartX;
	float DY = Y - Editor.DragStartY;
	if (ImGui::GetIO().KeyShift)   // Shift: 8픽셀 격자에 맞춤
	{
		DX = std::round(DX / 8.0f) * 8.0f;
		DY = std::round(DY / 8.0f) * 8.0f;
	}
	Editor.ApplyDrag(DX, DY);
}

void FUIEditorPanel::DrawPalette()
{
	ImGui::TextDisabled("Palette");
	TArray<UClass*> Classes;
	GetDerivedClasses(UWidget::StaticClass(), Classes);

	for (UClass* Class : Classes)
	{
		// 생성할 수 없는 클래스, 베이스 패널, UUserWidget 계열은 뺀다.
		if (!Class->Constructor || Class == UPanelWidget::StaticClass() || Class->IsChildOf(UUserWidget::StaticClass()))
			continue;
		if (!ImGui::Selectable(Class->Name.c_str())) continue;

		Editor.AddWidget(Class);
	}
}

void FUIEditorPanel::DrawHierarchy(UWidget* Widget)
{
	UPanelWidget* Panel = Cast<UPanelWidget>(Widget);

	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (!Panel || Panel->GetChildCount() == 0) Flags |= ImGuiTreeNodeFlags_Leaf;
	if (Widget == Editor.Selected) Flags |= ImGuiTreeNodeFlags_Selected;

	const FString Name = Widget->WidgetName.empty() ? Widget->GetClass()->Name : Widget->WidgetName;
	const FString Label = Name + "##" + std::to_string(Widget->GetUUID());
	const bool bOpen = ImGui::TreeNodeEx(Label.c_str(), Flags);
	if (ImGui::IsItemClicked()) Editor.Selected = Widget;

	// 끌어서 다른 패널 아래로 옮긴다.
	if (Widget != Editor.Root && ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload(PayloadType.c_str(), &Widget, sizeof(UWidget*));
		ImGui::TextUnformatted(Name.c_str());
		ImGui::EndDragDropSource();
	}
	if (Panel && ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(PayloadType.c_str()))
		{
			UWidget* Dropped = *static_cast<UWidget* const*>(Payload->Data);
			if (Editor.ContainsWidget(Dropped) && !IsSelfOrAncestor(Dropped, Panel))   // 자기 자손 아래로는 못 옮긴다
				Editor.PendingReparent = { Dropped, Panel };
		}
		ImGui::EndDragDropTarget();
	}

	if (bOpen)
	{
		if (Panel)
			for (UWidget* Child : Panel->GetChildren())
				DrawHierarchy(Child);
		ImGui::TreePop();
	}
}

void FUIEditorPanel::DrawDetails()
{
	if (!Editor.Selected)
	{
		ImGui::TextDisabled("Select a widget in the canvas or hierarchy.");
		return;
	}

	ImGui::TextUnformatted(Editor.Selected->GetClass()->Name.c_str());

	struct FAnchorPreset { const char* Label; FVector4 Anchors; };
	static const FAnchorPreset Presets[] = {
		{ "Top Left", {0, 0, 0, 0} }, { "Top Center", {0.5f, 0, 0.5f, 0} }, { "Top Right", {1, 0, 1, 0} },
		{ "Center Left", {0, 0.5f, 0, 0.5f} }, { "Center", {0.5f, 0.5f, 0.5f, 0.5f} }, { "Center Right", {1, 0.5f, 1, 0.5f} },
		{ "Bottom Left", {0, 1, 0, 1} }, { "Bottom Center", {0.5f, 1, 0.5f, 1} }, { "Bottom Right", {1, 1, 1, 1} },
		{ "Top Stretch", {0, 0, 1, 0} }, { "Bottom Stretch", {0, 1, 1, 1} }, { "Fill", {0, 0, 1, 1} },
	};
	if (Editor.Selected != Editor.Root && ImGui::BeginCombo("Anchors", "Select Preset"))
	{
		for (const FAnchorPreset& Preset : Presets)
			if (ImGui::Selectable(Preset.Label))
				Editor.SetAnchorsKeepRect(Editor.Selected, Preset.Anchors);
		ImGui::EndCombo();
	}

	FPropertyEditor::DrawObjectProperties(Editor.Selected);
}
