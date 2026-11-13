#pragma once

#include "EnginePCH.h"
#include "ParticleModule.generated.h"

struct FParticleEmitterInstance;
struct FBaseParticle;

UCLASS()
class UParticleModule : public UObject
{
	GENERATED_BODY()
public:
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle) {}

	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float deltaTime) {}
};
