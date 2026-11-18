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

		SpawnOrders.SetNum(MaxActiveParticles);
		SortedSlots.Reserve(MaxActiveParticles);

		return true;
	}

	void Tick(float deltaTime) override
	{
		int32 Before = ParticleCounter;
		FParticleEmitterInstance::Tick(deltaTime);
		int32 NewCount = ParticleCounter - Before;
		for (int i = 0;i < NewCount;i++)
		{
			int32 Slot = ParticleDataBlock.ParticleIndices[ActiveParticles - NewCount + i];	// ActiveParticles - NewCount + i => Tick 끝난 직후 ParticleIndices에서의 현재 Particle 위치
			SpawnOrders[Slot] = Before + i;													// 현재 파티클 위치에 전체 Spawn 기준으로 몇번째 Spawn인지 세팅, i는 NewCount를 따라가므로 순서를 그대로 따라감
		}
	}

	FDynamicEmitterDataBase* GetDynamicData() override;

protected:
	bool FillReplayData(FDynamicEmitterReplayDataBase& OutData) override;

private:
	TArray<uint32> SpawnOrders;	// Spawn 순서에 따른 배열
	TArray<int32> SortedSlots;	// 실제 그려야 되는 Ribbon 배열에 맞게 정렬할때 쓰는 배열

	TUniquePtr<FDynamicStripEmitterData> DynamicData;
};
