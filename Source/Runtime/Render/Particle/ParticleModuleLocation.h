#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleModuleLocation.generated.h"

UCLASS()
class UParticleModuleLocation : public UParticleModule
{
	GENERATED_BODY()
public:
	void Spawn() override;

private:
	FVector MinLocation = FVector(0, 0, 0);
	FVector MaxLocation = FVector(0, 10, 0);
};
