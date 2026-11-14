#pragma once

#include "Particles/ParticleEmitter.h"
#include "ParticleSystem.generated.h"

USTRUCT()
struct FParticleSystem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<FParticleEmitter> Emitters{ FParticleEmitter{} };
};