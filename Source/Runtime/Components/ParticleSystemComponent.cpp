#include "EnginePCH.h"
#include "Components/ParticleSystemComponent.h"
#include "Render/Particle/ParticleSystem.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleSpawn.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Engine/ParticleSystemSceneProxy.h"
#include "Render/Particle/ParticleStats.h"
#include "Render/Particle/ParticleModuleLifetime.h"
#include "Render/Particle/ParticleModuleLocation.h"
#include "Render/Particle/ParticleModuleVelocity.h"
#include "Render/Particle/ParticleModuleColor.h"
#include "Render/Particle/ParticleModuleColorOverLife.h"
#include "Render/Particle/ParticleModuleSize.h"
#include "Render/Material.h"
#include "Asset/AssetManager.h"
#include <algorithm>

namespace
{
	// [임시] 에디터가 생기기 전까지 쓰는 테스트용 에셋. 모듈을 추가할 때마다 여기도 늘어난다
	// 값은 이 엔진 단위(1 = 약 1m, SubUV 파티클 크기 1.2 정도)에 맞춘 것이다
	UParticleSystem* CreateTestParticleSystem()
	{
		UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
		Required->SetMaxParticles(300);
		Required->SetMaterial(UAssetManager::GetAssetByPath<UMaterial>("ParticleSpriteMaterial"));

		UParticleModuleSpawn* Spawn = NewObject<UParticleModuleSpawn>();
		Spawn->SetRate(60.0f);

		UParticleModuleLifetime* Lifetime = NewObject<UParticleModuleLifetime>();
		Lifetime->MinLifetime = 1.0f;
		Lifetime->MaxLifetime = 2.0f;

		UParticleModuleLocation* Location = NewObject<UParticleModuleLocation>();
		Location->StartLocationMin = FVector(-0.2f, -0.2f, 0.0f);
		Location->StartLocationMax = FVector( 0.2f,  0.2f, 0.0f);

		UParticleModuleVelocity* Velocity = NewObject<UParticleModuleVelocity>();
		Velocity->StartVelocityMin = FVector(-0.3f, -0.3f, 1.0f);
		Velocity->StartVelocityMax = FVector( 0.3f,  0.3f, 2.0f);

		UParticleModuleColor* Color = NewObject<UParticleModuleColor>();
		Color->StartColor = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);

		UParticleModuleColorOverLife* ColorOverLife = NewObject<UParticleModuleColorOverLife>();
		ColorOverLife->StartColor = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);
		ColorOverLife->EndColor   = FLinearColor(1.0f, 0.1f, 0.0f, 0.0f);   // 붉어지며 사라진다

		UParticleModuleSize* Size = NewObject<UParticleModuleSize>();
		Size->StartSizeMin = FVector(0.2f, 0.2f, 0.2f);
		Size->StartSizeMax = FVector(0.4f, 0.4f, 0.4f);

		UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
		LODLevel->SetRequiredModule(Required);
		LODLevel->SetSpawnModule(Spawn);
		LODLevel->AddModule(Lifetime);
		LODLevel->AddModule(Location);
		LODLevel->AddModule(Velocity);
		LODLevel->AddModule(Color);
		LODLevel->AddModule(ColorOverLife);
		LODLevel->AddModule(Size);

		UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
		Emitter->AddLODLevel(LODLevel);
		Emitter->CacheEmitterModuleInfo();   // 모듈을 다 넣은 뒤 Spawn/Update 목록 분류

		UParticleSystem* System = NewObject<UParticleSystem>();
		System->AddEmitter(Emitter);
		return System;
	}

	// 처음 필요할 때(첫 Tick) 한 번만 만든다. 생성자에서 만들면 엔진 초기화 전에 불릴 수 있다
	UParticleSystem* GetTestParticleSystem()
	{
		static UParticleSystem* TestSystem = CreateTestParticleSystem();
		return TestSystem;
	}
}

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
	if (Template == NewTemplate)
		return;
	Template = NewTemplate;
	InitializeSystem();   // 에디터에서도 바로 반영되도록 BeginPlay 여부와 상관없이 다시 만든다
}

void UParticleSystemComponent::InitializeSystem()
{
	ResetSystem();
	if (!Template)
		return;

	for (UParticleEmitter* Emitter : Template->GetEmitters())
	{
		if (!Emitter)
			continue;
		if (FParticleEmitterInstance* Instance = Emitter->CreateInstance(this))
			EmitterInstances.Add(Instance);
	}
}

void UParticleSystemComponent::ResetSystem()
{
	// 렌더 데이터는 인스턴스가 소유한다 → Proxy가 들고 있는 포인터를 먼저 비워서 지워진 메모리를 Submit하지 않게 한다
	if (FParticleSystemSceneProxy* Proxy = GetParticleSceneProxy())
		Proxy->ResetDynamicData();
	for (FParticleEmitterInstance* Instance : EmitterInstances)
		delete Instance;
	EmitterInstances.Reset();
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	if (!Template)
		Template = GetTestParticleSystem();   // [임시] 에디터가 생기면 지운다

	// 에디터 월드에는 BeginPlay가 없으므로 첫 Tick에서 지연 초기화한다
	if (EmitterInstances.IsEmpty() && Template)
		InitializeSystem();

	// 로딩 직후처럼 프레임이 크게 튀면 한꺼번에 쏟아지지 않도록 제한
	DeltaTime = std::min(DeltaTime, 0.1f);

	{
		SCOPE_CYCLE_COUNTER(ParticleStats::STAT_ParticleTick);
		for (FParticleEmitterInstance* Instance : EmitterInstances)
			Instance->Tick(DeltaTime);
		UpdateComponentBounds();
	}

	SCOPE_CYCLE_COUNTER(ParticleStats::STAT_ParticleRenderData);
	SendRenderDynamicData();
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

int32 UParticleSystemComponent::GetNumActiveParticles() const
{
	int32 Total = 0;
	for (const FParticleEmitterInstance* Instance : EmitterInstances)
		Total += Instance->ActiveParticles;
	return Total;
}

FBox UParticleSystemComponent::CalcLocalBounds() const
{
	return LocalBounds;
}
