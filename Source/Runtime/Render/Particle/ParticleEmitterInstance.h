#pragma once

#include "Render/Particle/ParticleHelper.h"   // 렌더 헤더는 include하지 않는다

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

	UParticleEmitter* SpriteTemplate = nullptr;   // 템플릿 (UE 이름 그대로)
	UParticleSystemComponent* Component = nullptr;        // 소유 컴포넌트
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
};