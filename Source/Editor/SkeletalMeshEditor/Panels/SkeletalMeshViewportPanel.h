#pragma once

#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshEditorPanel.h"

class FSkeletalMeshViewportPanel : public FSkeletalMeshEditorPanel
{
public:
	explicit FSkeletalMeshViewportPanel(FSkeletalMeshEditor& InEditor) : FSkeletalMeshEditorPanel(InEditor, "Viewport") {}

private:
	void DrawContent() override;
};
