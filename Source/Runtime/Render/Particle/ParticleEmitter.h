#pragma once

#include "EnginePCH.h"
#include "ParticleLODLevel.h"
#include "ParticleEmitter.generated.h"

UCLASS()
class UParticleEmitter : public UObject
{
	GENERATED_BODY()

private:
	TArray<UParticleLODLevel*> LODLevels;
};
