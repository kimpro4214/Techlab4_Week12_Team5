#pragma once

#include "Render/Particle/ParticleEmitterInstance.h"   // 렌더 헤더는 include하지 않는다

struct FDynamicBeamEmitterData;
class UParticleModuleTypeDataBeam;

// Beam 이미터의 실행 상태
// 파티클 대신 점 목록(Source → Target)을 렌더 쪽에 넘긴다
struct FParticleBeamEmitterInstance : public FParticleEmitterInstance
{
	explicit FParticleBeamEmitterInstance(UParticleModuleTypeDataBeam* InBeamTypeData);
	~FParticleBeamEmitterInstance() override;   // TUniquePtr<불완전 타입>이라 정의는 .cpp에 둔다

	FDynamicEmitterDataBase* GetDynamicData() override;

protected:
	bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override;

private:
	UParticleModuleTypeDataBeam* BeamTypeData = nullptr;
	TUniquePtr<FDynamicBeamEmitterData> DynamicData;
};
