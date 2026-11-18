#pragma once

#include "Render/Particle/ParticleEmitterInstance.h"

struct FDynamicStripEmitterData;

struct FParticleBeamEmitterInstance : public FParticleEmitterInstance
{
	FParticleBeamEmitterInstance();
	~FParticleBeamEmitterInstance() override;   

	bool Init(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent) override;

	FDynamicEmitterDataBase* GetDynamicData() override;

protected:
	bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override;

private:
	FVector TargetOffset   = FVector(100.0f, 0.0f, 0.0f);   // Emitter Local 기준 Target 위치
	int32   SegmentCount   = 8;                             // Beam 중간의 점, 중간 점을 Noise로 변화시켜 번개같은 효과를 부여
	float   NoiseAmplitude = 0.0f;                          // Segment가 옆으로 벗어나는 거리, 0이면 직선

	int32 MaxPoints = 0;   // 렌더 버퍼 용량 = MaxActiveParticles × (SegmentCount + 1). Init에서 계산

	TUniquePtr<FDynamicStripEmitterData> DynamicData;
};
