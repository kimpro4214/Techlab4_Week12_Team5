#include "EnginePCH.h"
#include "Editor/ParticleEditor/ParticleEditor.h"
#include "Editor/ParticleEditor/ParticleEditorTab.h"
#include "Asset/AssetSerializer.h"
#include "Core/FileUtils.h"
#include "UObject/UObjectIterator.h"
#include "Components/ParticleSystemComponent.h"
#include "Editor/Settings/SettingsPanel.h"
#include "Engine/World.h"
#include "Render/Particle/ParticleSystem.h"

FParticleEditor::FParticleEditor(UEngine* InEngine, FEditorManager& InManager, const FAssetEditorKey& InKey,
	UParticleSystem* InSystem, const FEditorSettings* InSettings, FPreviewRenderer InRenderer)
	: FAssetEditor(InManager, InKey)
	, Engine(InEngine)
	, SourceSystem(InSystem)
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
	PreviewScene.Tick(bViewportVisible && bPreviewValid ? DeltaTime : 0.0f);
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

bool FParticleEditor::IsDirty() const
{
	if (!System) return false;

	TArray<uint8> Snapshot;
	FString Error;
	if (!FAssetSerializer::SaveToBytes(System, Snapshot, Error)) return true;

	return Snapshot.Num() != SavedSnapshot.Num() ||
		std::memcmp(Snapshot.GetData(), SavedSnapshot.GetData(), Snapshot.Num()) != 0;
}

bool FParticleEditor::Save()
{
	if (!CanSave()) return false;

	TArray<uint8> Snapshot;
	if (!FAssetSerializer::SaveToBytes(System, Snapshot, LastError)) return false;

	// 파일 교체 전에 복원 가능한 데이터와 공유 원본에 반영할 객체를 준비합니다.
	TUniquePtr<UParticleSystem> SavedSystem(Cast<UParticleSystem>(
		FAssetSerializer::LoadFromBytes(Snapshot, UParticleSystem::StaticClass(), LastError)));
	if (!SavedSystem) return false;

	TArray<UParticleSystemComponent*> Components;
	for (TObjectIterator<UParticleSystemComponent> Component; Component; ++Component)
	{
		if (Component->GetTemplate() == SourceSystem) Components.Add(*Component);
	}

	if (!FFileUtils::WriteAllBytesAtomic(GetKey().AssetPath, Snapshot, LastError))
	{
		HTR_LOG(Error, "Cannot save particle '{}': {}", GetKey().AssetPath, LastError);
		return false;
	}

	// 실행 인스턴스가 이전 모듈을 참조하지 않도록 먼저 해제합니다.
	for (UParticleSystemComponent* Component : Components) Component->ResetSystem();
	SourceSystem->SwapEmitters(*SavedSystem);
	for (UParticleSystemComponent* Component : Components) Component->Activate(true);

	SavedSnapshot = std::move(Snapshot);
	LastError.clear();
	return true;
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
	RefreshSystem();
}

////////////////////////////////////////////////////////////
// 이미터·모듈 선택 및 변경 반영
////////////////////////////////////////////////////////////

void FParticleEditor::SelectEmitter(int32 Index)
{
	SelectedEmitterIndex = System && System->GetEmitters().IsValidIndex(Index) ? Index : -1;
	SelectedModule = nullptr;
}

void FParticleEditor::AddEmitter()
{
	if (!System) return;

	System->AddDefaultEmitter();
	SelectLODLevel(0);
	SelectEmitter(System->GetEmitters().Num() - 1);
	RefreshSystem();
}

int32 FParticleEditor::GetLODLevelCount() const
{
	int32 Count = 0;
	if (!System) return Count;

	for (const UParticleEmitter* Emitter : System->GetEmitters())
	{
		if (Emitter && Emitter->GetLODLevels().Num() > Count)
		{
			Count = Emitter->GetLODLevels().Num();
		}
	}
	return Count;
}

void FParticleEditor::SelectLODLevel(int32 Index)
{
	if (Index < 0 || (Index != 0 && Index >= GetLODLevelCount()) || Index == SelectedLODLevel)
	{
		return;
	}

	SelectedLODLevel = Index;
	SelectedModule = nullptr;
}

void FParticleEditor::SelectModule(int32 EmitterIndex, UParticleModule* Module)
{
	SelectEmitter(EmitterIndex);
	UParticleEmitter* Emitter = GetSelectedEmitter();
	const UParticleLODLevel* LOD = Emitter ? Emitter->GetLODLevel(SelectedLODLevel) : nullptr;
	if (!LOD || !Module) return;

	if (Module == LOD->GetRequiredModule() || Module == LOD->GetSpawnModule() ||
		LOD->GetModules().Find(Module) >= 0)
	{
		SelectedModule = Module;
	}
}

UParticleEmitter* FParticleEditor::GetSelectedEmitter() const
{
	return System && System->GetEmitters().IsValidIndex(SelectedEmitterIndex)
		? System->GetEmitters()[SelectedEmitterIndex] : nullptr;
}

bool FParticleEditor::RefreshSystem()
{
	if (!System || !ParticleComponent) return false;

	// 캐시와 메모리 배치를 바꾸기 전에 기존 인스턴스를 해제합니다.
	ParticleComponent->ResetSystem();
	bPreviewValid = System->ValidateAsset(LastError);
	if (!bPreviewValid) return false;

	ParticleComponent->Activate(true);
	LastError.clear();
	return true;
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
	if (!SourceSystem || !FAssetSerializer::SaveToBytes(SourceSystem, SavedSnapshot, LastError))
	{
		return false;
	}

	System = Cast<UParticleSystem>(FAssetSerializer::LoadFromBytes(
		SavedSnapshot, UParticleSystem::StaticClass(), LastError));
	if (!System) return false;

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
	SelectEmitter(System->GetEmitters().IsEmpty() ? -1 : 0);
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

	SelectEmitter(-1);
	delete System;
	System = nullptr;
	SourceSystem = nullptr;
	SavedSnapshot.Reset();
}

bool FParticleEditor::DiscardChanges()
{
	if (!System) return true;

	TUniquePtr<UParticleSystem> Restored(Cast<UParticleSystem>(
		FAssetSerializer::LoadFromBytes(SavedSnapshot, UParticleSystem::StaticClass(), LastError)));
	if (!Restored) return false;

	// 복원할 객체를 준비한 뒤 교체하여 복원 실패 시 편집 상태를 유지합니다.
	if (ParticleComponent) ParticleComponent->SetTemplate(nullptr);
	SelectEmitter(-1);
	delete System;
	System = Restored.release();
	SelectLODLevel(0);
	bPreviewValid = true;
	if (ParticleComponent) ParticleComponent->SetTemplate(System);

	LastError.clear();
	return true;
}
