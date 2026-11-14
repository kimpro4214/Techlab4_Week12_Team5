#pragma once

#include "Particles/ParticleEmitterInstance.h"
#include "ParticleModuleVelocity.generated.h"

USTRUCT()
struct FParticleModuleVelocity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FVector InitialVelocity = FVector(0.0f, 0.0f, 1.0f);

	void Spawn(FBaseParticle& Particle) const
	{
		Particle.Velocity = InitialVelocity;
	}
};
