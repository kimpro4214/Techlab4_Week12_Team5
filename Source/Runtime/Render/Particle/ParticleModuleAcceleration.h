#pragma once

#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleAcceleration.generated.h"

UCLASS()
class UParticleModuleAcceleration : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleAcceleration() { bUpdateModule = true; }

	void Update(const FUpdateContext& Context) override;

	// 시뮬레이션 축 기준으로 초당 더할 속도입니다.
	UPROPERTY(EditAnywhere) FVector Acceleration = FVector(0.0f, 0.0f, 0.0f);
};
