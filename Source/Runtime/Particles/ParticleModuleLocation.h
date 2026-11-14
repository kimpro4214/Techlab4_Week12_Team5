#pragma once

#include "Particles/ParticleEmitterInstance.h"
#include "ParticleModuleLocation.generated.h"

USTRUCT()
struct FParticleModuleLocation
{
	GENERATED_BODY()

	// 컴포넌트 원점에서 월드 축 기준으로 적용할 오프셋.
	UPROPERTY(EditAnywhere)
	FVector Offset = FVector(0.0f, 0.0f, 0.0f);

	void Spawn(FParticle& Particle, const FVector& Origin) const
	{
		Particle.Location = Origin + Offset;
	}
};
