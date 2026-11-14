#pragma once

#include "Particles/ParticleEmitterInstance.h"
#include "ParticleModuleLifetime.generated.h"

USTRUCT()
struct FParticleModuleLifetime
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta=(ClampMin=0.001))
	float Lifetime = 2.0f;

	void Spawn(FBaseParticle& Particle) const
	{
		Particle.LifeTime = Lifetime;
	}
};
