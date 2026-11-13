#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleModuleSpawn.generated.h"

UCLASS()
class UParticleModuleSpawn : public UParticleModule
{
	GENERATED_BODY()
private:
	float Speed;
	float SpeedScale;
};
