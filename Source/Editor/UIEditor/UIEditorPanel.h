#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include "Editor/UIEditor/UIEditor.h"

class FUIEditorPanel : public IEditorPanel
{
public:
	enum class ERole { Palette, Hierarchy, Canvas, Details };
	FUIEditorPanel(FUIEditor& InEditor, ERole InRole, const char* InName);
	bool Init() override { return true; }
	void Tick(float) override {}
	void OnRender() override;
	const char* GetPanelName() const override { return PanelName.c_str(); }
	const char* GetWindowName() const { return WindowName.c_str(); }
	void SetDockClass(ImGuiID InId) { DockClassId = InId; }
	bool IsDetached(ImGuiID DockSpaceId) const;

private:
	void DrawPalette();
	void DrawHierarchy(UWidget* Widget);
	void DrawCanvas();
	void DrawDetails();
	void HandleCanvasInput(bool bCanvasHovered);
	bool IsOverResizeHandle(float X, float Y) const;
	ImVec2 ToScreen(float X, float Y) const { return ImVec2(CanvasMin.x + X * ViewScale, CanvasMin.y + Y * ViewScale); }

	FUIEditor& Editor;
	ERole Role;
	FString PanelName;
	FString WindowName;
	FString PayloadType;
	ImGuiID DockClassId = 0;
	ImVec2 CanvasMin;
	float ViewScale = 1.0f;
};
