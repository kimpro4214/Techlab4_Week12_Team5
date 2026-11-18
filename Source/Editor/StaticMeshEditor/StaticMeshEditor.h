#pragma once

#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/AssetEditor/FPreviewScene.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Camera/ViewportCameraMovement.h"
#include <functional>

class UEngine;
class UStaticMesh;
class UStaticMeshComponent;
struct FEditorSettings;

class FStaticMeshEditor : public FAssetEditor
{
public:
	using FPreviewRenderer = std::function<void(FStaticMeshEditor&)>;
	FStaticMeshEditor(UEngine* InEngine, FEditorManager& InManager, const FAssetEditorKey& InKey,
		UStaticMesh* InMesh, const FEditorSettings* InSettings, FPreviewRenderer InRenderer);
	~FStaticMeshEditor() override;

	static UStaticMesh* LoadMesh(const FString& Path, FString& OutError);
	void Tick(float DeltaTime) override;
	void RenderPreview() override;
	void OnRender(bool) override {}
	bool IsFocused() const override { return bFocused; }
	void BeginUIFrame();
	void ReportPanelFocus(bool bInFocused) { bFocused |= bInFocused; }
	void FocusMesh();
	void SetViewportState(uint32 Width, uint32 Height, const FViewportCameraInput& Input);
	bool IsViewportVisible() const { return bViewportVisible; }
	bool IsRenderGrid() const { return bRenderGrid; }
	void SetRenderGrid(bool bValue) { bRenderGrid = bValue; }
	UStaticMesh* GetMesh() const { return Mesh; }
	UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }
	UWorld* GetPreviewWorld() const { return PreviewScene.GetWorld(); }
	FEditorViewportClient* GetViewportClient() { return &ViewportClient; }

protected:
	TUniquePtr<IEditorTab> CreateTab() override;
	bool InitializeResources() override;
	void ShutdownResources() override;

private:
	UEngine* Engine = nullptr;
	UStaticMesh* Mesh = nullptr;
	UStaticMeshComponent* MeshComponent = nullptr;
	const FEditorSettings* Settings = nullptr;
	FPreviewRenderer PreviewRenderer;
	FPreviewScene PreviewScene;
	FEditorViewportClient ViewportClient;
	FViewportCameraInput CameraInput;
	uint32 ViewportWidth = 0;
	uint32 ViewportHeight = 0;
	bool bViewportVisible = false;
	bool bFocused = false;
	bool bRenderGrid = false;
	bool bInitialFocusPending = true;
};
