#pragma once

#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleModuleSpawn.h"
#include "Particles/ParticleModuleLifetime.h"
#include "ParticleEmitter.generated.h"

USTRUCT()
struct FParticleEmitter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FParticleModuleRequired Required;

	UPROPERTY(EditAnywhere)
	FParticleModuleSpawn Spawn;

	FParticleModuleLifetime Lifetime;
};
