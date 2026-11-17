#include "EnginePCH.h"
#include "StaticMeshDetailsPanel.h"
#include "Editor/StaticMeshEditor/StaticMeshEditor.h"
#include "Render/Mesh.h"

FStaticMeshDetailsPanel::FStaticMeshDetailsPanel(FStaticMeshEditor& InEditor)
	: FStaticMeshEditorPanel(InEditor, "Details")
{
}

void FStaticMeshDetailsPanel::DrawContent()
{
	const UStaticMesh* Mesh = Editor.GetMesh();
	if (!Mesh) return;
	const FBox& Bounds = Mesh->GetRenderBounds();
	ImGui::TextDisabled("Render Bounds");
	ImGui::Text("Min: %.2f, %.2f, %.2f", Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z);
	ImGui::Text("Max: %.2f, %.2f, %.2f", Bounds.Max.X, Bounds.Max.Y, Bounds.Max.Z);
	ImGui::SeparatorText("Material Slots");
	for (const auto& Slot : Mesh->GetMeshData().MaterialSlots)
		ImGui::TextUnformatted(Slot.Name.c_str());
}
