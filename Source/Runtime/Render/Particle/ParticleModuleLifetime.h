#pragma once
#include "Render/Particle/ParticleModule.h"
#include "ParticleHelper.h"
#include "ParticleModuleLifetime.generated.h"

UCLASS()
class UParticleModuleLifetime : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleLifetime() { bSpawnModule = true; }
	void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
	{
		const float Life = ParticleRandRange(MinLifetime, MaxLifetime);
		ParticleBase->OneOverMaxLifetime = Life > 0.0f ? 1.0f / Life : 0.0f;
	}

	UPROPERTY(EditAnywhere) float MinLifetime = 1.0f;
	UPROPERTY(EditAnywhere) float MaxLifetime = 1.0f;
};