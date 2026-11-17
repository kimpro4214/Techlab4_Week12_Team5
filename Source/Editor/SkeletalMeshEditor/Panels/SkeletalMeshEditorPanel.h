#pragma once

#include "Containers/Array.h"
#include "Core/EngineString.h"
#include "Editor/EditorUI/EditorPanel.h"

class FSkeletalMeshEditor;

class FSkeletalMeshEditorPanel : public IEditorPanel
{
public:
	FSkeletalMeshEditorPanel(FSkeletalMeshEditor& InEditor, const char* InName);
	bool Init() override { return true; }
	void Tick(float) override {}
	void OnRender() override;
	const char* GetPanelName() const override { return PanelName.c_str(); }
	const char* GetWindowName() const { return WindowName.c_str(); }
	void SetDockClass(ImGuiID InClassId) { DockClassId = InClassId; }
	bool IsDetached(ImGuiID DockSpaceId) const;

protected:
	virtual void DrawContent() = 0;
	FSkeletalMeshEditor& Editor;

private:
	FString PanelName;
	FString WindowName;
	ImGuiID DockClassId = 0;
};
