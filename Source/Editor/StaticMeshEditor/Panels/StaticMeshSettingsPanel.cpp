#include "EnginePCH.h"
#include "StaticMeshSettingsPanel.h"
#include "Editor/StaticMeshEditor/StaticMeshEditor.h"

FStaticMeshSettingsPanel::FStaticMeshSettingsPanel(FStaticMeshEditor& InEditor)
	: FStaticMeshEditorPanel(InEditor, "Settings")
{
}

void FStaticMeshSettingsPanel::DrawContent()
{
	bool bGrid = Editor.IsRenderGrid();
	if (ImGui::Checkbox("Grid", &bGrid)) Editor.SetRenderGrid(bGrid);
	if (FEditorViewportClient* Client = Editor.GetViewportClient())
	{
		bool bWireframe = Client->IsWireframe();
		if (ImGui::Checkbox("Wireframe", &bWireframe))
		{
			Client->SetViewportMode(bWireframe ? EViewportMode::Wireframe : EViewportMode::Solid);
		}
	}
}
