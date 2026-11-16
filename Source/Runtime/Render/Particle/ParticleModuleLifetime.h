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
	void Spawn(const FSpawnContext& Context) override
	{
		const float Life = ParticleRandRange(MinLifetime, MaxLifetime);
		Context.ParticleBase->OneOverMaxLifetime = Life > 0.0f ? 1.0f / Life : 0.0f;
	}

	UPROPERTY(EditAnywhere) float MinLifetime = 1.0f;
	UPROPERTY(EditAnywhere) float MaxLifetime = 1.0f;
};
