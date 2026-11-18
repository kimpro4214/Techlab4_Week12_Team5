#include "EnginePCH.h"
#include "Editor/UIEditor/UIEditorTab.h"
#include "Editor/UIEditor/UIEditor.h"
#include "Core/FileUtils.h"
#include "UI/UserWidget.h"
#include "UObject/UObjectHash.h"

FUIEditorTab::FUIEditorTab(FUIEditor& InEditor)
	: FAssetEditorTab(InEditor), UIEditor(InEditor)
{
	const ImGuiID AssetId = ImHashStr(InEditor.GetKey().GetTabId().c_str());
	DockSpaceId = ImHashStr("UIDockSpace", 0, AssetId);
	PanelClassId = ImHashStr("UIPanelClass", 0, AssetId);
	Panels.Add(MakeUnique<FUIEditorPanel>(InEditor, FUIEditorPanel::ERole::Palette, "Palette"));
	Panels.Add(MakeUnique<FUIEditorPanel>(InEditor, FUIEditorPanel::ERole::Hierarchy, "Hierarchy"));
	Panels.Add(MakeUnique<FUIEditorPanel>(InEditor, FUIEditorPanel::ERole::Canvas, "Canvas"));
	Panels.Add(MakeUnique<FUIEditorPanel>(InEditor, FUIEditorPanel::ERole::Details, "Details"));
	for (auto& Panel : Panels)
	{
		Panel->SetDockClass(PanelClassId);
		Panel->Init();
	}
}

void FUIEditorTab::DrawMenuBar()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Save", "Ctrl+S", false, UIEditor.CanSave())) UIEditor.Save();
		if (ImGui::MenuItem("Close Editor")) UIEditor.Close();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "Ctrl+Z", false, !UIEditor.UndoStack.IsEmpty())) UIEditor.Undo();
		if (ImGui::MenuItem("Redo", "Ctrl+Y", false, !UIEditor.RedoStack.IsEmpty())) UIEditor.Redo();
		ImGui::EndMenu();
	}
	DrawToolbar();
	if (ImGui::BeginMenu("Window"))
	{
		for (auto& Panel : Panels)
		{
			if (ImGui::MenuItem(Panel->GetPanelName(), nullptr, Panel->IsOpen())) Panel->SetOpen(!Panel->IsOpen());
		}
		ImGui::EndMenu();
	}
}

void FUIEditorTab::BuildDefaultLayout(const ImVec2& Size)
{
	ImGui::DockBuilderAddNode(DockSpaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(DockSpaceId, Size);
	ImGuiID CanvasId = DockSpaceId;
	const ImGuiID LeftId = ImGui::DockBuilderSplitNode(CanvasId, ImGuiDir_Left, 0.20f, nullptr, &CanvasId);
	const ImGuiID DetailsId = ImGui::DockBuilderSplitNode(CanvasId, ImGuiDir_Right, 0.28f, nullptr, &CanvasId);
	ImGuiID PaletteId = LeftId;
	const ImGuiID HierarchyId = ImGui::DockBuilderSplitNode(PaletteId, ImGuiDir_Down, 0.6f, nullptr, &PaletteId);
	ImGui::DockBuilderDockWindow(Panels[0]->GetWindowName(), PaletteId);
	ImGui::DockBuilderDockWindow(Panels[1]->GetWindowName(), HierarchyId);
	ImGui::DockBuilderDockWindow(Panels[2]->GetWindowName(), CanvasId);
	ImGui::DockBuilderDockWindow(Panels[3]->GetWindowName(), DetailsId);
	ImGui::DockBuilderFinish(DockSpaceId);
}

void FUIEditorTab::OnRender(bool bActive)
{
	UIEditor.BeginUIFrame();
	if (UIEditor.IsClosing()) return;
	UIEditor.ReportPanelFocus(bActive && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));

	const ImVec2 Size = ImGui::GetContentRegionAvail();
	if (bActive && Size.x > 0.0f && Size.y > 0.0f && !ImGui::DockBuilderGetNode(DockSpaceId))
		BuildDefaultLayout(Size);

	ImGuiWindowClass WindowClass;
	WindowClass.ClassId = PanelClassId;
	WindowClass.DockingAllowUnclassed = false;
	if (bActive || ImGui::DockBuilderGetNode(DockSpaceId))
	{
		const ImGuiDockNodeFlags Flags = bActive ? ImGuiDockNodeFlags_None : ImGuiDockNodeFlags_KeepAliveOnly;
		ImGui::DockSpace(DockSpaceId, ImVec2(0.0f, 0.0f), Flags, &WindowClass);
	}
	for (auto& Panel : Panels)
	{
		if (Panel->IsOpen() && (bActive || Panel->IsDetached(DockSpaceId))) Panel->OnRender();
	}
	HandleShortcuts();
	UIEditor.EndUIFrame(ImGui::IsAnyItemActive());
}

void FUIEditorTab::DrawToolbar()
{
	// 앵커가 맞는지 다른 비율에서 확인한다.
	static const FVector2 Sizes[] = { {1920, 1080}, {1280, 720}, {2560, 1080}, {1080, 1920} };
	const FString SizeLabel = std::format("{}x{}", (int)UIEditor.PreviewSize.X, (int)UIEditor.PreviewSize.Y);
	if (ImGui::BeginMenu(SizeLabel.c_str()))
	{
		for (const FVector2& Size : Sizes)
			if (ImGui::MenuItem(std::format("{}x{}", (int)Size.X, (int)Size.Y).c_str()))
				UIEditor.PreviewSize = Size;
		ImGui::EndMenu();
	}

	// 이 에셋으로 만들 C++ 클래스 (UUserWidget 자손)
	if (ImGui::BeginMenu(("Parent: " + UIEditor.ParentClassName).c_str()))
	{
		TArray<UClass*> Classes;
		GetDerivedClasses(UUserWidget::StaticClass(), Classes);
		Classes.Add(UUserWidget::StaticClass());
		for (UClass* Class : Classes)
			if (ImGui::MenuItem(Class->Name.c_str(), nullptr, Class->Name == UIEditor.ParentClassName))
			{
				UIEditor.ParentClassName = Class->Name;
			}
		ImGui::EndMenu();
	}

	const FString Name = FFileUtils::ToUTF8String(FFileUtils::ToNativePath(UIEditor.GetKey().AssetPath).filename());
	ImGui::TextDisabled("%s%s", Name.c_str(), UIEditor.IsDirty() ? " *" : "");
}

void FUIEditorTab::HandleShortcuts()
{
	if (!UIEditor.IsFocused() || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive() ||
		ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) return;
	const ImGuiIO& IO = ImGui::GetIO();
	if (IO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) UIEditor.Save();
	if (IO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) IO.KeyShift ? UIEditor.Redo() : UIEditor.Undo();
	if (IO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) UIEditor.Redo();
	if (ImGui::IsKeyPressed(ImGuiKey_Delete)) UIEditor.DeleteSelected();
}
