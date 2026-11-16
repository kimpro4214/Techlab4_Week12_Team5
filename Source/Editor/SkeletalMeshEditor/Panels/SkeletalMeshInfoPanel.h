#pragma once

#include "Editor/SkeletalMeshEditor/Panels/SkeletalMeshEditorPanel.h"

class FSkeletalMeshInfoPanel : public FSkeletalMeshEditorPanel
{
public:
	explicit FSkeletalMeshInfoPanel(FSkeletalMeshEditor& InEditor) : FSkeletalMeshEditorPanel(InEditor, "Skeleton Info") {}

private:
	void DrawContent() override;
};
