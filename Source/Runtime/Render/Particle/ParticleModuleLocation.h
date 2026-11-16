#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleModuleLocation.generated.h"

UCLASS()
class UParticleModuleLocation : public UParticleModule
{
	GENERATED_BODY()
public:
	void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle) override;

	FVector GetMinLocation() const { return MinLocation; }
	FVector GetMaxLocation() const { return MaxLocation; }

private:
	FVector MinLocation = FVector(0, 0, 0);
	FVector MaxLocation = FVector(0, 10, 0);
};
