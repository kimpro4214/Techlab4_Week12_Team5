#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleModuleSize.generated.h"

UCLASS()
class UParticleModuleSize : public UParticleModule
{
	GENERATED_BODY()
public:
	void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle) override;

	FVector GetMinSize() const { return MinSize; }
	FVector GetMaxSize() const { return MaxSize; }

private:
	FVector MinSize = FVector(25, 25, 25);
	FVector MaxSize = FVector(25, 25, 25);
};
