#pragma once

#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleModuleSpawn.h"
#include "Particles/ParticleModuleLifetime.h"
#include "Particles/ParticleModuleLocation.h"
#include "Particles/ParticleModuleVelocity.h"
#include "Particles/ParticleModuleSize.h"
#include "Particles/ParticleModuleColor.h"
#include "ParticleEmitter.generated.h"

USTRUCT()
struct FParticleEmitter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FParticleModuleRequired Required;

	UPROPERTY(EditAnywhere)
	FParticleModuleSpawn Spawn;

	UPROPERTY(EditAnywhere)
	FParticleModuleLifetime Lifetime;

	UPROPERTY(EditAnywhere)
	FParticleModuleLocation Location;

	UPROPERTY(EditAnywhere)
	FParticleModuleVelocity Velocity;

	UPROPERTY(EditAnywhere)
	FParticleModuleSize Size;

	UPROPERTY(EditAnywhere)
	FParticleModuleColor Color;
};
