#pragma once

#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshEditorPanel.h"

class FSkeletalMeshSettingsPanel : public FSkeletalMeshEditorPanel
{
public:
	explicit FSkeletalMeshSettingsPanel(FSkeletalMeshEditor& InEditor) : FSkeletalMeshEditorPanel(InEditor, "Preview Settings") {}

private:
	void DrawContent() override;
};
