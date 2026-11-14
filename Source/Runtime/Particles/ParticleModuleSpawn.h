#pragma once

#include <algorithm>

#include "Core/Types.h"

#include "ParticleModuleSpawn.generated.h"

USTRUCT()
struct FParticleModuleSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0))
	float Rate = 5.0f;

	int32 GetSpawnCount(float DeltaTime, float& SpawnRemainder) const
	{
		SpawnRemainder += std::max(0.0f, Rate) * DeltaTime;

		const int32 SpawnCount = static_cast<int32>(SpawnRemainder);
		SpawnRemainder -= static_cast<float>(SpawnCount);
		return SpawnCount;
	}
};