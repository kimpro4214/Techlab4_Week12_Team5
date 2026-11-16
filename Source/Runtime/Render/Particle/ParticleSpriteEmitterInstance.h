#pragma once

#include "Render/Particle/ParticleEmitterInstance.h"   // 렌더 헤더는 include하지 않는다

struct FDynamicSpriteEmitterData;

// 스프라이트(카메라를 향하는 사각형) 이미터의 실행 상태 
// 시뮬레이션은 부모와 같고, 렌더 데이터를 만드는 부분만 다르다
struct FParticleSpriteEmitterInstance : public FParticleEmitterInstance
{
	FParticleSpriteEmitterInstance();
	~FParticleSpriteEmitterInstance() override;   // TUniquePtr<불완전 타입>이라 정의는 .cpp에 둔다

	FDynamicEmitterDataBase* GetDynamicData() override;

protected:
	bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override;

private:
	// GPU 버퍼를 매 프레임 새로 만들지 않도록 인스턴스가 들고 있는다
	TUniquePtr<FDynamicSpriteEmitterData> DynamicData;
};
