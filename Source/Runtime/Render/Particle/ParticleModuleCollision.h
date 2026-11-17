#pragma once

#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleCollision.generated.h"

UCLASS()
class UParticleModuleCollision : public UParticleModule
{
	GENERATED_BODY()

public:
	UParticleModuleCollision() { bUpdateModule = true; }

	void Update(const FUpdateContext& Context) override;
};