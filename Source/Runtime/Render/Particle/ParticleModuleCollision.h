#pragma once

#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleCollision.generated.h"

UCLASS()
class UParticleModuleCollision : public UParticleModule
{
	GENERATED_BODY()

public:
	UParticleModuleCollision()
	{
		bSpawnModule = true;
		bUpdateModule = true;
	}

	void Spawn(const FSpawnContext& Context) override;
	void Update(const FUpdateContext& Context) override;
};
