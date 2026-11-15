#include "EnginePCH.h"
#include "Components/ParticleSystemComponent.h"
#include "Render/Particle/ParticleSystem.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleSpawn.h"
#include <algorithm>

namespace
{
	// [임시] 에디터가 생기기 전까지 쓰는 테스트용 에셋. 모듈을 추가할 때마다 여기도 늘어난다
	UParticleSystem* CreateTestParticleSystem()
	{
		UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
		Required->SetMaxParticles(300);

		UParticleModuleSpawn* Spawn = NewObject<UParticleModuleSpawn>();
		Spawn->SetRate(60.0f);

		UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
		LODLevel->SetRequiredModule(Required);
		LODLevel->SetSpawnModule(Spawn);

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

	for (FParticleEmitterInstance* Instance : EmitterInstances)
		Instance->Tick(DeltaTime);

	// [임시] 렌더 전까지 시뮬레이션을 로그로 확인한다
	DebugLogTimer += DeltaTime;
	if (DebugLogTimer >= 1.0f)
	{
		DebugLogTimer = 0.0f;
		HTR_LOG(Info, "[Particle] Emitters: {}, Active: {}", EmitterInstances.Num(), GetNumActiveParticles());
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
	return FBox{ FVector(-BoundsExtent, -BoundsExtent, -BoundsExtent), FVector(BoundsExtent, BoundsExtent, BoundsExtent) };
}
