#pragma once

#include "Render/Particle/ParticleEmitterInstance.h"   // 렌더 헤더는 include하지 않는다

struct FDynamicStripEmitterData;

struct FParticleRibbonEmitterInstance : public FParticleEmitterInstance
{
	FParticleRibbonEmitterInstance();
	~FParticleRibbonEmitterInstance() override;

	bool Init(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent) override
	{
		if (!FParticleEmitterInstance::Init(InTemplate, InComponent)) return false;

		SortedSlots.Reserve(MaxActiveParticles);

		return true;
	}

	// Tick은 오버라이드하지 않는다. 생성 순서는 SpawnParticles가 각 파티클에 넣는 ParticleId(= ParticleCounter++)로 알 수 있다

	FDynamicEmitterDataBase* GetDynamicData() override;

protected:
	bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override;

private:
	TArray<int32> SortedSlots;	// 살아있는 파티클의 칸 번호를 생성 순서(최신 → 오래된)로 정렬한 임시 배열

	TUniquePtr<FDynamicStripEmitterData> DynamicData;
};
