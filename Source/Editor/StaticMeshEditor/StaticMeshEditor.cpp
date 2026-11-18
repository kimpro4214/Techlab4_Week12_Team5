#include "EnginePCH.h"
#include "Editor/StaticMeshEditor/StaticMeshEditor.h"
#include "Editor/StaticMeshEditor/StaticMeshEditorTab.h"
#include "Asset/AssetManager.h"
#include "Components/StaticMeshComponent.h"
#include "Editor/Settings/SettingsPanel.h"
#include "Engine/World.h"
#include "Render/Mesh.h"

#include <cctype>

FStaticMeshEditor::FStaticMeshEditor(UEngine* InEngine, FEditorManager& InManager, const FAssetEditorKey& InKey,
	UStaticMesh* InMesh, const FEditorSettings* InSettings, FPreviewRenderer InRenderer)
	: FAssetEditor(InManager, InKey), Engine(InEngine), Mesh(InMesh), Settings(InSettings),
	PreviewRenderer(std::move(InRenderer))
{
}

FStaticMeshEditor::~FStaticMeshEditor()
{
	Shutdown();
}

UStaticMesh* FStaticMeshEditor::LoadMesh(const FString& Path, FString& OutError)
{
	OutError.clear();
	FString Extension = fs::path(Path).extension().string();
	std::transform(Extension.begin(), Extension.end(), Extension.begin(),
		[](unsigned char Character) { return static_cast<char>(std::tolower(Character)); });
	if (Extension == ".obj")
	{
		UStaticMesh* Loaded = UAssetManager::LoadObjStaticMesh(Path);
		if (!Loaded) OutError = "Cannot load OBJ static mesh: " + Path;
		return Loaded;
	}
	if (Extension == ".fbx")
	{
		std::error_code Error;
		const fs::path Relative = fs::relative(Path, "Assets", Error);
		if (!Error) return UAssetManager::Get().LoadFbxStaticMesh(Relative.generic_string(), Path, OutError);
		OutError = Error.message();
		return nullptr;
	}
	OutError = "Unsupported static mesh format: " + Extension;
	return nullptr;
}

TUniquePtr<IEditorTab> FStaticMeshEditor::CreateTab()
{
	return MakeUnique<FStaticMeshEditorTab>(*this);
}

bool FStaticMeshEditor::InitializeResources()
{
	if (!Mesh || Mesh->GetMeshData().Vertices.IsEmpty() || Mesh->GetMeshData().Indices.IsEmpty()) return false;
	FPreviewSceneOptions Options;
	Options.WorldName = "StaticMeshPreview";
	if (!PreviewScene.Initialize(Engine, Options)) return false;

	ViewportClient.SetWorld(PreviewScene.GetWorld());
	MeshComponent = Cast<UStaticMeshComponent>(PreviewScene.AddComponent(UStaticMeshComponent::StaticClass(), "PreviewMesh"));
	if (!MeshComponent) return false;
	MeshComponent->SetStaticMesh(Mesh);
	PreviewScene.Tick(0.0f);
	FocusMesh();
	return true;
}

void FStaticMeshEditor::ShutdownResources()
{
	BeginUIFrame();
	ViewportClient.SetWorld(nullptr);
	ViewportClient.GetViewTargets().Reset();
	MeshComponent = nullptr;
	PreviewScene.Release();
	Mesh = nullptr;
}

void FStaticMeshEditor::FocusMesh()
{
	if (Mesh) ViewportClient.FocusBounds(Mesh->GetRenderBounds());
}

void FStaticMeshEditor::BeginUIFrame()
{
	bViewportVisible = false;
	bFocused = false;
	CameraInput = {};
}

void FStaticMeshEditor::SetViewportState(uint32 Width, uint32 Height, const FViewportCameraInput& Input)
{
	ViewportWidth = Width;
	ViewportHeight = Height;
	bViewportVisible = Width > 0 && Height > 0;
	CameraInput = bViewportVisible ? Input : FViewportCameraInput{};
}

void FStaticMeshEditor::Tick(float DeltaTime)
{
	if (bViewportVisible && ViewportClient.GetWorld())
	{
		FViewportCameraState Camera{
			.Location = ViewportClient.GetViewLocation(),
			.Rotation = ViewportClient.GetViewRotation(),
			.OrthoWidth = ViewportClient.GetOrthoWidth(),
			.ViewWidth = static_cast<float>(ViewportWidth),
			.bPerspective = true
		};
		const float MoveSpeed = 5.0f * (Settings ? Settings->CameraSpeed : 1.0f);
		const float Sensitivity = 0.2f * (Settings ? Settings->MouseSensitivity : 1.0f);
		ApplyViewportCameraMovement(Camera, CameraInput, DeltaTime, MoveSpeed, Sensitivity);
		ViewportClient.SetViewLocation(Camera.Location);
		ViewportClient.SetViewRotation(Camera.Rotation);
		ViewportClient.SetOrthoWidth(Camera.OrthoWidth);
	}
	CameraInput = {};
	// 정적 프리뷰는 게임 Tick 대신 변환만 반영한다.
	PreviewScene.Tick(DeltaTime);
}

void FStaticMeshEditor::RenderPreview()
{
	if (!bViewportVisible || !ViewportClient.GetWorld() || IsClosing()) return;
	ViewportClient.Resize(ViewportWidth, ViewportHeight);
	if (!ViewportClient.GetViewTargets().IsValidFor(ViewportWidth, ViewportHeight)) return;
	if (bInitialFocusPending)
	{
		FocusMesh();
		bInitialFocusPending = false;
	}
	if (PreviewRenderer) PreviewRenderer(*this);
}
