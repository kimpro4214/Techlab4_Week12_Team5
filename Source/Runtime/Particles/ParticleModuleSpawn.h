#pragma once

#include <algorithm>

#include "Core/Types.h"

struct FParticleModuleSpawn
{
	float Rate = 5.0f;

	int32 GetSpawnCount(float DeltaTime, float& SpawnRemainder) const
	{
		SpawnRemainder += std::max(0.0f, Rate) * DeltaTime;

		const int32 SpawnCount = static_cast<int32>(SpawnRemainder);
		SpawnRemainder -= static_cast<float>(SpawnCount);
		return SpawnCount;
	}
};