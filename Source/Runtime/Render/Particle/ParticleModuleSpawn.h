#pragma once

#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleSpawn.generated.h"

// 생성 빈도 
// EmitterInstance가 값을 직접 읽는다. Spawn/Update 호출 대상이 아니다.
UCLASS()
class UParticleModuleSpawn : public UParticleModule
{
	GENERATED_BODY()
public:
	float GetRate() const { return Rate; }
	float GetRateScale() const { return RateScale; }
	int32 GetBurstCount() const { return BurstCount; }

	void SetRate(float InRate) { Rate = InRate; }
	void SetRateScale(float InScale) { RateScale = InScale; }
	void SetBurstCount(int32 InCount) { BurstCount = InCount; }

private:
	float Rate = 20.0f;      // 초당 생성 수
	float RateScale = 1.0f;  // Rate × RateScale = 실제 초당 생성 수
	int32 BurstCount = 0;    // Emitter 시작 시 한 번에 생성할 수
};