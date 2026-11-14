#pragma once

#include "ParticleModuleRequired.generated.h"

USTRUCT()
struct FParticleModuleRequired
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta=(ClampMin=0.0f))
	float Duration = 0.0f;
};