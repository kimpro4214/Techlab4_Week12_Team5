#pragma once

#include "EnginePCH.h"
#include "ParticleEmitter.h"
#include "ParticleSystem.generated.h"

UCLASS()
class UParticleSystem : public UObject
{
	GENERATED_BODY()

private:
	TArray<UParticleEmitter*> Emitters;
};
