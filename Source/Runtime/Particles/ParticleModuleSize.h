#pragma once

#include "Particles/ParticleEmitterInstance.h"
#include "ParticleModuleSize.generated.h"

USTRUCT()
struct FParticleModuleSize
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta=(ClampMin=0.0))
	float InitialSize = 1.0f;

	void Spawn(FParticle& Particle) const
	{
		Particle.Scale = InitialSize;
	}
};
