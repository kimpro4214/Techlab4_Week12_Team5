#pragma once

#include "Render/Particle/ParticleHelper.h"   // 렌더 헤더는 include하지 않는다
#include "Render/Particle/ParticleModule.h"
#include "Components/ParticleSystemComponent.h"

class UParticleEmitter;
class UParticleLODLevel;
class UParticleSystemComponent;

// Component × Emitter 하나마다 하나 존재하는 실행 상태 
// Payload 블록을 소유하고 Spawn / Update / Kill을 수행한다.
struct FParticleEmitterInstance
{
	FParticleEmitterInstance() = default;
	virtual ~FParticleEmitterInstance();

	// 블록을 소유하므로 복사 금지 (복사되면 같은 블록을 두 번 해제한다)
	FParticleEmitterInstance(const FParticleEmitterInstance&) = delete;
	FParticleEmitterInstance& operator=(const FParticleEmitterInstance&) = delete;

	// 템플릿과 컴포넌트를 연결하고 블록을 할당한다. Required/Spawn이 없으면 실패
	virtual bool Init(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent);

	// 이번 프레임 생성 개수를 계산해 SpawnParticles를 부른다. 소수점은 SpawnFraction으로 이월
	void Spawn(float DeltaTime);
	// Count개를 빈 칸에 생성한다. 블록이 가득 차면 멈춘다
	void SpawnParticles(int32 Count, const FVector& InitialLocation);

	// 매 Tick 시작에 갱신
	void UpdateTransform();

	FMatrix EmitterToSimulation = FMatrix::Identity;   // Emitter 로컬 → 시뮬레이션 공간, 원점이 생성 위치
	FMatrix SimulationToWorld = FMatrix::Identity;   // 시뮬레이션 공간 → 월드, 렌더 단계에서 사용

	UParticleEmitter* SpriteTemplate = nullptr;   // 템플릿 
	UParticleSystemComponent* Component = nullptr;   // 소유 컴포넌트
	int32 CurrentLODLevelIndex = 0;
	UParticleLODLevel* CurrentLODLevel = nullptr;

	FParticleDataContainer ParticleDataBlock;
	int32  ParticleSize = 0;        // 파티클 1개의 실제 크기 (지금은 sizeof(FBaseParticle))
	int32  ParticleStride = 0;      // 칸 간격. ParticleSize를 16의 배수로 올린 값
	int32  ActiveParticles = 0;     // 살아있는 파티클 수 = ParticleIndices 앞쪽 유효 개수
	int32  MaxActiveParticles = 0;
	uint32 ParticleCounter = 0;     // 지금까지 생성한 총 개수 (단조 증가)
	float  SpawnFraction = 0.0f;    // 지난 프레임에 못 만든 소수점 몫
	float  EmitterTime = 0.0f;

protected:
	bool AllocateParticleData(int32 MaxParticles);
	void FreeParticleData();

	// 새 파티클의 칸을 비우고 기본값을 채운다 (모듈이 덮어쓰지 않은 값은 이 값을 쓴다)
	void PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation);
	// 파티클이 생성될 위치. LocalSpace면 컴포넌트 로컬, 아니면 월드 좌표
	FVector GetSpawnOrigin() const;
};