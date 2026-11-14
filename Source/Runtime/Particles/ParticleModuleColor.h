#pragma once

#include "Math/Color.h"
#include "Particles/ParticleEmitterInstance.h"
#include "ParticleModuleColor.generated.h"

USTRUCT()
struct FParticleModuleColor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FLinearColor InitialColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	void Spawn(FParticle& Particle) const
	{
		Particle.Color = FVector(InitialColor.R, InitialColor.G, InitialColor.B);
		Particle.Alpha = InitialColor.A;
	}
};
