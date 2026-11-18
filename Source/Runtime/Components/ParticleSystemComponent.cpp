#include "EnginePCH.h"
#include "Components/ParticleSystemComponent.h"
#include "Render/Particle/ParticleSystem.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Engine/ParticleSystemSceneProxy.h"
#include "Render/Particle/ParticleStats.h"
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
	if (Template != NewTemplate)
	{
		SetPreviewEmitterFilter(0, nullptr);
	}
	Template = NewTemplate;
	InitializeSystem();   // 에디터에서도 바로 반영되도록 BeginPlay 여부와 상관없이 다시 만든다
}

void UParticleSystemComponent::SetPreviewEmitterFilter(int32 LODIndex, const UParticleEmitter* SoloEmitter)
{
	EmitterLODIndex = LODIndex;
	PreviewSoloEmitter = SoloEmitter;
}

int32 UParticleSystemComponent::GetNumActiveParticles() const
{
	int32 Count = 0;
	for (const FParticleEmitterInstance* Instance : EmitterInstances)
	{
		Count += Instance->ActiveParticles;
	}

	return Count;
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

	bSystemInitialized = true;
	InitializedTemplate = Template;
	for (UParticleEmitter* Emitter : Template->GetEmitters())
	{
		const UParticleLODLevel* LOD = Emitter ? Emitter->GetLODLevel(EmitterLODIndex) : nullptr;
		if (!LOD || (PreviewSoloEmitter ? Emitter != PreviewSoloEmitter : !LOD->IsEnabled()))
			continue;
		Emitter->CacheEmitterModuleInfo(EmitterLODIndex);
		if (FParticleEmitterInstance* Instance = Emitter->CreateInstance(this))
			EmitterInstances.Add(Instance);
	}
}

void UParticleSystemComponent::ResetSystem()
{
	bSystemInitialized = false;

	// 렌더 데이터는 인스턴스가 소유한다 → Proxy가 들고 있는 포인터를 먼저 비워서 지워진 메모리를 Submit하지 않게 한다
	if (FParticleSystemSceneProxy* Proxy = GetParticleSceneProxy())
		Proxy->ResetDynamicData();
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
	// 씬 로드는 생성자가 기본 Template으로 만든 뒤 Template 값만 덮어쓰므로, 바뀌었으면 다시 만든다
	const bool bTemplateChanged = bSystemInitialized && Template != InitializedTemplate;
	if ((!bSystemInitialized || bTemplateChanged) && Template && !bPendingDeactivate)
		InitializeSystem();

	// 로딩 직후처럼 프레임이 크게 튀면 한꺼번에 쏟아지지 않도록 제한
	DeltaTime = std::min(DeltaTime, 0.1f);

	const UWorld* World = GetWorld();
	// 파티클 에디터에서는 상단에서 선택한 LOD로 시뮬레이션한다.
	const ACameraActor* MainCamera = World && World->GetWorldType() != EWorldType::EditorPreview
		? World->GetMainCamera() : nullptr;
	const UCameraComponent* Camera = MainCamera ? MainCamera->GetCameraComponent() : nullptr;

	const float CameraDistance = Camera ? (Camera->GetWorldLocation() - GetWorldLocation()).Length() : 0.0f;

	{
		SCOPE_CYCLE_COUNTER(ParticleStats::STAT_ParticleTick);
		for (FParticleEmitterInstance* Instance : EmitterInstances)
		{
			if (Camera && Template)
			{
				const int32 CurrentLOD = Instance->CurrentLODLevelIndex;
				const int32 LODCount = Instance->SpriteTemplate->GetLODLevels().Num();
				for (int32 Index = LODCount - 1; Index >= 0; --Index)
				{
					// 가까워지는 방향에 여유를 두어 경계에서 반복 전환되는 것을 줄입니다.
					const float Distance = Template->GetLODDistance(Index);
					const float Gap = Distance - Template->GetLODDistance(Index - 1);
					const float Margin = std::clamp(Gap * 0.5f, 0.0f, 1.0f);
					const float SwitchDistance = Index <= CurrentLOD ? Distance - Margin : Distance;
					if (CameraDistance >= SwitchDistance && (Index == CurrentLOD || Instance->SetCurrentLODIndex(Index)))
						break;
				}
			}
			Instance->Tick(DeltaTime);
		}
		// 모든 이미터의 Tick이 끝나 이번 프레임 충돌이 다 모인 뒤에 알린다
		DispatchCollisionEvents();
		UpdateComponentBounds();
	}

	{
		SCOPE_CYCLE_COUNTER(ParticleStats::STAT_ParticleRenderData);
		SendRenderDynamicData();
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
		// 렌더 데이터는 Proxy가 들고 있으므로 Proxy 쪽을 비운다
		if (FParticleSystemSceneProxy* Proxy = GetParticleSceneProxy())
			Proxy->ResetDynamicData();
		Super::Deactivate();
	}
}

void UParticleSystemComponent::RefreshRenderData()
{
	// 프리뷰가 일시정지되어 있어도 이동한 원점을 렌더링에 반영한다.
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		Instance->UpdateTransforms();
	}

	SendRenderDynamicData();
}

void UParticleSystemComponent::DispatchCollisionEvents()
{
	// 1) 이미터마다 Tick 중에 쌓인 이번 프레임 충돌을 한곳에 모은다 (이미터는 다음 Tick 시작에 자기 목록을 비운다)
	CollisionEvents.Reset();
	for (const FParticleEmitterInstance* Instance : EmitterInstances)
		CollisionEvents.Append(Instance->GetCollisionEvents());

	if (CollisionEvents.IsEmpty())
		return;

	// 2) 바깥에 알린다. 구독한 쪽이 없으면 아무 일도 하지 않는다
	for (const FParticleCollisionEvent& Event : CollisionEvents)
		OnParticleCollide.Broadcast(this, Event);
}

void UParticleSystemComponent::UpdateComponentBounds()
{
	FBox NewBounds;
	bool bHasBounds = false;

	if (Template && Template->IsUseFixedRelativeBoundingBox())
	{
		// 에셋에 정해둔 박스를 그대로 쓴다. 매 프레임 계산하지 않아 싸지만, 벗어난 파티클은 같이 컬링된다
		NewBounds  = Template->GetFixedRelativeBoundingBox();
		bHasBounds = true;
	}
	else
	{
		const FMatrix WorldToComponent = GetWorldMatrix().Inverse();
		for (const FParticleEmitterInstance* Instance : EmitterInstances)
		{
			FBox EmitterBox;
			if (!Instance->GetLocalBoundingBox(WorldToComponent, EmitterBox))
				continue;

			if (!bHasBounds)
			{
				NewBounds  = EmitterBox;
				bHasBounds = true;
				continue;
			}
			NewBounds.Min.X = std::min(NewBounds.Min.X, EmitterBox.Min.X);  NewBounds.Max.X = std::max(NewBounds.Max.X, EmitterBox.Max.X);
			NewBounds.Min.Y = std::min(NewBounds.Min.Y, EmitterBox.Min.Y);  NewBounds.Max.Y = std::max(NewBounds.Max.Y, EmitterBox.Max.Y);
			NewBounds.Min.Z = std::min(NewBounds.Min.Z, EmitterBox.Min.Z);  NewBounds.Max.Z = std::max(NewBounds.Max.Z, EmitterBox.Max.Z);
		}
	}

	// 그릴 것이 없으면 이전 Bounds를 유지한다. 다음 프레임 첫 파티클이 컬링으로 사라지지 않게
	if (!bHasBounds)
		return;

	// BVH 갱신을 매 프레임 하지 않도록: 박스를 벗어나면 바로 키우고, 많이 줄었을 때만 줄인다
	const auto Contains = [](const FBox& Outer, const FBox& Inner)
	{
		return Inner.Min.X >= Outer.Min.X && Inner.Min.Y >= Outer.Min.Y && Inner.Min.Z >= Outer.Min.Z
			&& Inner.Max.X <= Outer.Max.X && Inner.Max.Y <= Outer.Max.Y && Inner.Max.Z <= Outer.Max.Z;
	};
	const auto Volume = [](const FBox& Box)
	{
		const FVector Size = Box.GetSize();
		return Size.X * Size.Y * Size.Z;
	};

	const bool bGrew   = !Contains(LocalBounds, NewBounds);
	const bool bShrank = Volume(NewBounds) < Volume(LocalBounds) * 0.25f;
	if (!bGrew && !bShrank)
		return;

	// 조금씩 커질 때마다 다시 갱신하지 않도록 여유를 둔다 (축마다 10% + 0.1)
	const FVector Margin = NewBounds.GetSize() * 0.1f + FVector(0.1f, 0.1f, 0.1f);
	LocalBounds = FBox(NewBounds.Min - Margin, NewBounds.Max + Margin);

	// Proxy가 CalcLocalBounds로 월드 Bounds를 다시 구하고 BVH에 반영한다
	OnTransformDirty();
}

FPrimitiveSceneProxy* UParticleSystemComponent::CreateSceneProxy()
{
	return new FParticleSystemSceneProxy(this);
}

FParticleSystemSceneProxy* UParticleSystemComponent::GetParticleSceneProxy() const
{
	// CreateSceneProxy가 항상 FParticleSystemSceneProxy를 만드므로 static_cast로 충분하다
	return static_cast<FParticleSystemSceneProxy*>(GetSceneProxy());
}

void UParticleSystemComponent::SendRenderDynamicData()
{
	// 렌더 상태가 아직 없으면(등록 전) 넘길 곳이 없다. 다음 Tick에 다시 만든다
	FParticleSystemSceneProxy* Proxy = GetParticleSceneProxy();
	if (!Proxy)
		return;

	// 시뮬레이션이 끝난 뒤 프레임당 한 번만 만든다
	// 뷰마다 만들면 안 된다: GetDynamicData가 뷰 버퍼 인덱스를 처음으로 되돌린다
	Proxy->ResetDynamicData();
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (FDynamicEmitterDataBase* Data = Instance->GetDynamicData())
			Proxy->AddDynamicData(Data);
	}
}

FBox UParticleSystemComponent::CalcLocalBounds() const
{
	return LocalBounds;
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
