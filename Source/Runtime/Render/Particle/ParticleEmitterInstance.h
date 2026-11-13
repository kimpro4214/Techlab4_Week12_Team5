#pragma once

#include "EnginePCH.h"
#include "ParticleHelper.h"
#include "ParticleEmitter.h"
#include "Core/HAL/UnrealMemory.h"

class UParticleSystemComponent;


// Component × Emitter마다 1개씩 존재하는 Emitter의 실행 상태
// 파티클 메모리를 보유하고 Spawn / Update / Kill을 수행
struct FParticleEmitterInstance
{
	FParticleEmitterInstance() = default;
	~FParticleEmitterInstance();	// 할당한 파티클 메모리 블록 해제

	// 메모리 블록을 소유하므로 복사 금지 (복사되면 같은 블록을 두 번 해제함)
	FParticleEmitterInstance(const FParticleEmitterInstance&) = delete;
	FParticleEmitterInstance& operator=(const FParticleEmitterInstance&) = delete;

	bool Init(UParticleEmitter* Emitter, UParticleSystemComponent* ParticleComponent);

	void Spawn(float DeltaTime);          // 이번 프레임 생성 개수 계산 → 남은 소수 반환
	void SpawnParticles(int32 Count);      // Count개를 실제로 생성

	void Update()
	{

	}

	UParticleEmitter* SpriteTemplate = nullptr;		// 실제 Emitter 참조로 Module 가져옴
	UParticleSystemComponent* Component = nullptr;	// Component에 정보를 전달하기 위한 참조
	UParticleLODLevel* CurrentLODLevel = nullptr;	// 현재 Emitter의 LODLevel
	FParticleDataContainer ParticleDataBlock;	// 파티클 데이터를 담는 Block
	
	int32 ParticleSize;	// Particle 자체의 사이즈, 
	int32 ParticleStride;	// Particle Size가 16단위가 아닐 경우 맞추기 위한 간격, ex) Size=84 -> Stride=96
	int32 ActiveParticles; // 활성화된 파티클 개수
	int32 MaxActiveParticles;	// 가능한 파티클 최대 개수
	float SpawnFraction;	//다음 프레임으로 넘길 생성량 소수점
};
