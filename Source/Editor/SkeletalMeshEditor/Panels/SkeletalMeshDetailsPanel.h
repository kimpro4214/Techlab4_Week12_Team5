#pragma once

#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshEditorPanel.h"

class FSkeletalMeshDetailsPanel : public FSkeletalMeshEditorPanel
{
public:
	explicit FSkeletalMeshDetailsPanel(FSkeletalMeshEditor& InEditor) : FSkeletalMeshEditorPanel(InEditor, "Details") {}

private:
	void DrawContent() override;
};
