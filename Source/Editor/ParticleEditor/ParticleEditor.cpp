#include "EnginePCH.h"
#include "Editor/ParticleEditor/ParticleEditor.h"
#include "Editor/ParticleEditor/ParticleEditorTab.h"
#include "Asset/AssetManager.h"
#include "Components/ParticleSystemComponent.h"
#include "Editor/Settings/SettingsPanel.h"
#include "Engine/World.h"
#include "Render/Particle/ParticleSystem.h"

FParticleEditor::FParticleEditor(UEngine* InEngine, FEditorManager& InManager, const FAssetEditorKey& InKey,
	UParticleSystem* InSystem, const FEditorSettings* InSettings, FPreviewRenderer InRenderer)
	: FAssetEditor(InManager, InKey)
	, Engine(InEngine)
	, System(InSystem)
	, Settings(InSettings)
	, PreviewRenderer(std::move(InRenderer))
{
}

FParticleEditor::~FParticleEditor()
{
	Shutdown();
}

////////////////////////////////////////////////////////////
// 에디터 공통 인터페이스
////////////////////////////////////////////////////////////

void FParticleEditor::Tick(float DeltaTime)
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

	// 분리된 뷰포트를 포함해 실제로 표시되는 동안만 시뮬레이션한다.
	PreviewScene.Tick(bViewportVisible ? DeltaTime : 0.0f);
}

void FParticleEditor::RenderPreview()
{
	if (!bViewportVisible || !ViewportClient.GetWorld() || IsClosing())
	{
		return;
	}

	ViewportClient.Resize(ViewportWidth, ViewportHeight);
	if (!ViewportClient.GetViewTargets().IsValidFor(ViewportWidth, ViewportHeight))
	{
		return;
	}

	if (bInitialFocusPending)
	{
		FocusSystem();
		bInitialFocusPending = false;
	}

	// 일시정지 중에도 뷰 버퍼를 재사용하여 카메라 이동과 반복 렌더링을 지원한다.
	ParticleComponent->RefreshRenderData();

	if (PreviewRenderer)
	{
		PreviewRenderer(*this);
	}
}

////////////////////////////////////////////////////////////
// 패널 상태 및 뷰포트 제어
////////////////////////////////////////////////////////////

void FParticleEditor::BeginUIFrame()
{
	bViewportVisible = false;
	bFocused = false;
	CameraInput = {};
}

void FParticleEditor::SetViewportState(uint32 Width, uint32 Height, const FViewportCameraInput& Input)
{
	ViewportWidth = Width;
	ViewportHeight = Height;
	bViewportVisible = Width > 0 && Height > 0;
	CameraInput = bViewportVisible ? Input : FViewportCameraInput{};
}

void FParticleEditor::FocusSystem()
{
	ViewportClient.FocusBounds(FBox{ FVector(-2.0f, -2.0f, 0.0f), FVector(2.0f, 2.0f, 4.0f) });
}

////////////////////////////////////////////////////////////
// 프리뷰 시뮬레이션 제어
////////////////////////////////////////////////////////////

void FParticleEditor::Restart()
{
	if (ParticleComponent)
	{
		ParticleComponent->Activate(true);
	}
}

////////////////////////////////////////////////////////////
// Getter
////////////////////////////////////////////////////////////

int32 FParticleEditor::GetActiveParticleCount() const
{
	return ParticleComponent ? ParticleComponent->GetNumActiveParticles() : 0;
}

////////////////////////////////////////////////////////////
// 탭 생성 및 리소스 수명 관리
////////////////////////////////////////////////////////////

TUniquePtr<IEditorTab> FParticleEditor::CreateTab()
{
	return MakeUnique<FParticleEditorTab>(*this);
}

bool FParticleEditor::InitializeResources()
{
	if (!System)
	{
		return false;
	}

	FPreviewSceneOptions Options;
	Options.WorldName = "ParticlePreview";
	Options.bSimulate = true;

	if (!PreviewScene.Initialize(Engine, Options))
	{
		return false;
	}

	ViewportClient.SetWorld(PreviewScene.GetWorld());
	ParticleComponent = Cast<UParticleSystemComponent>(
		PreviewScene.AddComponent(UParticleSystemComponent::StaticClass(), "PreviewParticle"));
	if (!ParticleComponent)
	{
		return false;
	}

	ParticleComponent->SetTemplate(System);
	PreviewScene.Tick(0.0f);
	FocusSystem();

	return true;
}

void FParticleEditor::ShutdownResources()
{
	BeginUIFrame();
	ViewportClient.SetWorld(nullptr);
	ViewportClient.GetViewTargets().Reset();

	ParticleComponent = nullptr;
	PreviewScene.Release();

	System = nullptr;
}
