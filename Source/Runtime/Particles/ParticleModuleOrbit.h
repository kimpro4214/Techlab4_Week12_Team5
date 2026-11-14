#pragma once

#include "Math/Vector.h"
#include "ParticleModuleOrbit.generated.h"

struct FOrbitParticlePayload
{
	float AngleRadians = 0.0f;
	FVector Offset = FVector(0.0f, 0.0f, 0.0f);
};

USTRUCT()
struct FParticleModuleOrbit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	bool bEnabled = false;
};