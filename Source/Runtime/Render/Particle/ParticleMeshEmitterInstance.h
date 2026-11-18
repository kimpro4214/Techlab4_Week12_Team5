#pragma once

#include "Render/Particle/ParticleEmitterInstance.h"   // 렌더 헤더는 include하지 않는다

struct FDynamicMeshEmitterData;
class UParticleModuleTypeDataMesh;

// 파티클마다 메시를 그리는 이미터의 실행 상태
// 시뮬레이션은 부모와 같고, 렌더 데이터를 만드는 부분만 다르다
struct FParticleMeshEmitterInstance : public FParticleEmitterInstance
{
	explicit FParticleMeshEmitterInstance(UParticleModuleTypeDataMesh* InMeshTypeData);
	~FParticleMeshEmitterInstance() override;   // TUniquePtr<불완전 타입>이라 정의는 .cpp에 둔다

	FDynamicEmitterDataBase* GetDynamicData() override;

protected:
	bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override;
	float GetParticleExtent(const FBaseParticle& Particle) const override;   // 메시 반지름 × 파티클 크기

private:
	UParticleModuleTypeDataMesh* MeshTypeData = nullptr;   // 메시와 방향 설정
	TUniquePtr<FDynamicMeshEmitterData> DynamicData;
};
