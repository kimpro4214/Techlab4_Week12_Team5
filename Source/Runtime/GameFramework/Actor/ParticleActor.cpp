#include "EnginePCH.h"
#include "ParticleActor.h"
#include "Render/Particle/ParticleModules.h"

namespace
{
	// [임시] 에디터/에셋 로딩이 생기기 전까지 쓰는 테스트용 Template
	// 조립 순서: 모듈 → LODLevel → Emitter → System (아래에서 위로)
	// 모듈 값은 각 클래스의 기본값을 그대로 사용한다
	UParticleSystem* BuildTestParticleSystem()
	{
		UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
		UParticleModuleSpawn* Spawn = NewObject<UParticleModuleSpawn>();

		UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
		LODLevel->SetRequiredModule(Required);		// Required/Spawn은 전용 필드로 보관 (Modules 배열에는 넣지 않음)
		LODLevel->SetSpawnModule(Spawn);

		// Modules 배열 순서 = Spawn/Update 실행 순서
		LODLevel->AddModule(NewObject<UParticleModuleLifetime>());	// 빠지면 Lifetime=0 → 파티클이 죽지 않음
		LODLevel->AddModule(NewObject<UParticleModuleLocation>());
		LODLevel->AddModule(NewObject<UParticleModuleVelocity>());
		LODLevel->AddModule(NewObject<UParticleModuleColor>());
		LODLevel->AddModule(NewObject<UParticleModuleSize>());

		UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
		Emitter->AddLODLevels(LODLevel);

		UParticleSystem* System = NewObject<UParticleSystem>();
		System->AddEmitter(Emitter);
		return System;
	}

	UParticleSystem* GetTestParticleSystem()	// 테스트용 기본 파티클 시스템
	{
		static UParticleSystem* TestSystem = BuildTestParticleSystem();
		return TestSystem;
	}
}

AParticleActor::AParticleActor()
{
	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>("UParticleSystemComponent");
	ParticleComponent->SetupAttachment(GetRootComponent());

	// Template만 꽂는다. Instance 생성(InitializeSystem)은 Component가 BeginPlay에서 수행
	ParticleComponent->SetTemplate(GetTestParticleSystem());
}
