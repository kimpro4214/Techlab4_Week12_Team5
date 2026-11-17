#include "EnginePCH.h"
#include "Components/ParticleSystemComponent.h"
#include "Render/Particle/ParticleSystem.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Render/Material.h"
#include "Asset/AssetManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include <algorithm>

UParticleSystemComponent::UParticleSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bCastShadow = false;   // 반투명 스프라이트는 그림자를 만들지 않는다
}

UParticleSystemComponent::~UParticleSystemComponent()
{
	ResetSystem();
}

void UParticleSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeSystem();
}

void UParticleSystemComponent::SetTemplate(UParticleSystem* NewTemplate)
{
	Template = NewTemplate;
	InitializeSystem();   // 에디터에서도 바로 반영되도록 BeginPlay 여부와 상관없이 다시 만든다
}

void UParticleSystemComponent::InitializeSystem()
{
	ResetSystem();

	// 인스턴스를 새로 만들면 새 생애가 시작된다 → 이전 생애의 끄는 중/완료 상태를 지운다
	// 지우지 않으면 완료 후 다시 켜도 Tick 첫머리에서 return 해서 아무것도 나오지 않는다
	bPendingDeactivate = false;
	bHasCompleted = false;

	if (!Template)
		return;

	for (UParticleEmitter* Emitter : Template->GetEmitters())
	{
		if (!Emitter)
			continue;
		Emitter->CacheEmitterModuleInfo();
		if (FParticleEmitterInstance* Instance = Emitter->CreateInstance(this))
			EmitterInstances.Add(Instance);
	}
}

void UParticleSystemComponent::ResetSystem()
{
	// 렌더 데이터는 인스턴스가 소유한다 → 인스턴스보다 먼저 비워서 지워진 메모리를 Submit하지 않게 한다
	EmitterRenderData.Reset();
	for (FParticleEmitterInstance* Instance : EmitterInstances)
		delete Instance;
	EmitterInstances.Reset();
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
	if (bHasCompleted) return;

	Super::TickComponent(DeltaTime);

	if (!Template)
	{
		FString Error;
		Template = UAssetManager::Get().LoadParticleSystem("Assets/Particles/Default.particle", Error);
	}

	// 에디터 월드에는 BeginPlay가 없으므로 첫 Tick에서 지연 초기화한다
	// 끄는 중에는 만들지 않는다: 첫 Tick 전에 Deactivate가 불렸는데 여기서 만들면 다시 뿜기 시작한다
	if (EmitterInstances.IsEmpty() && Template && !bPendingDeactivate)
		InitializeSystem();

	// 로딩 직후처럼 프레임이 크게 튀면 한꺼번에 쏟아지지 않도록 제한
	DeltaTime = std::min(DeltaTime, 0.1f);

	const UWorld* World = GetWorld();
	const ACameraActor* MainCamera = World ? World->GetMainCamera() : nullptr;
	const UCameraComponent* Camera = MainCamera ? MainCamera->GetCameraComponent() : nullptr;

	const float CameraDistance = Camera ? (Camera->GetWorldLocation() - GetWorldLocation()).Length() : 0.0f;

	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Camera && Template && Instance->SpriteTemplate->GetLODLevel(1)) 
		{
			const float FarDistance = Template->GetLOD1Distance();
			const float ReturnDistance = std::max(0.0f, FarDistance - 1.0f);
			const int32 CurrentLOD = Instance->CurrentLODLevelIndex;

			const float SwitchDistance = (CurrentLOD == 0) ? FarDistance : ReturnDistance;
			const int32 DesiredLOD = (CameraDistance >= SwitchDistance) ? 1 : 0;

			if (DesiredLOD != CurrentLOD)
				Instance->SetCurrentLODIndex(DesiredLOD);
		}
		Instance->Tick(DeltaTime);
	}

	// 시뮬레이션이 끝난 뒤 프레임당 한 번 렌더 데이터를 만든다
	// 뷰마다 만들면 안 된다: GetDynamicData가 뷰 버퍼 인덱스를 처음으로 되돌린다
	EmitterRenderData.Reset();
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (FDynamicEmitterDataBase* Data = Instance->GetDynamicData())
			EmitterRenderData.Add(Data);
	}

	// [임시] 렌더 전까지 시뮬레이션을 로그로 확인한다
	DebugLogTimer += DeltaTime;
	if (DebugLogTimer >= 1.0f)
	{
		DebugLogTimer = 0.0f;
		const int32 RenderParticles = EmitterRenderData.IsEmpty()
			? 0 : EmitterRenderData[0]->GetSource().ActiveParticleCount;
		HTR_LOG(Info, "[Particle] Emitters: {}, Active: {}, RenderData: {}, FirstEmitterRenderCount: {}",
			EmitterInstances.Num(), GetNumActiveParticles(), EmitterRenderData.Num(), RenderParticles);
	}

	bool bAllCompleted = !EmitterInstances.IsEmpty(); 
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (!Instance->HasCompleted())
		{
			bAllCompleted = false;
			break;
		}
	}
	if (bAllCompleted)
	{
		bHasCompleted = true;
		bPendingDeactivate = false;
		EmitterRenderData.Reset();
		Super::Deactivate(); 
	}
}

void UParticleSystemComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext)
{
	// 뷰포트(4분할 등)마다 한 번씩 불린다
	for (FDynamicEmitterDataBase* Data : EmitterRenderData)
		Data->Submit(RenderQueue, ViewContext);
}

int32 UParticleSystemComponent::GetNumActiveParticles() const
{
	int32 Total = 0;
	for (const FParticleEmitterInstance* Instance : EmitterInstances)
		Total += Instance->ActiveParticles;
	return Total;
}

FBox UParticleSystemComponent::CalcLocalBounds() const
{
	return FBox{ FVector(-BoundsExtent, -BoundsExtent, -BoundsExtent), FVector(BoundsExtent, BoundsExtent, BoundsExtent) };
}

void UParticleSystemComponent::Deactivate()
{
	if (bPendingDeactivate) return;
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		Instance->StopSpawning();
	}
	bPendingDeactivate = true;
}

void UParticleSystemComponent::Activate(bool bReset)
{
	bool IsEmitting = (EmitterInstances.Num() != 0 && !bPendingDeactivate && !bHasCompleted);
	if (IsEmitting && !bReset) return;

	InitializeSystem();
	Super::Activate(true);
}

void UParticleSystemComponent::PostEditChangeProperty(const FProperty& Property)
{
	Super::PostEditChangeProperty(Property);
	if (Property.Name == "Template")
		InitializeSystem();
}
