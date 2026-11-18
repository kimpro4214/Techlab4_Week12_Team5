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
	bool GetLocalBoundingBox(const FMatrix& WorldToComponent, FBox& OutBox) const override;   // 파티클이 아니라 점 기준

protected:
	bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override;

private:
	// 시작점과 끝점의 월드 좌표
	void GetEndpoints(FVector& OutSource, FVector& OutTarget) const;

	UParticleModuleTypeDataBeam* BeamTypeData = nullptr;
	TUniquePtr<FDynamicBeamEmitterData> DynamicData;
};
