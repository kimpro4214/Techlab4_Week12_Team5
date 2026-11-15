#include "EnginePCH.h"
#include "ParticleModule.h"
#include "Math/Vector.h"

#include <cmath>

bool UParticleModule::IsFiniteVector(const FVector& Value)
{
	return std::isfinite(Value.X) && std::isfinite(Value.Y) && std::isfinite(Value.Z);
}

bool UParticleModuleRequired::Validate() const
{
	return MaxActiveParticles > 0 && MaxActiveParticles <= 65535 &&
		std::isfinite(EmitterDuration) && EmitterDuration > 0.0f && EmitterLoops >= 0;
}

bool UParticleModuleSpawn::Validate() const
{
	return std::isfinite(SpawnRate) && SpawnRate >= 0.0f;
}
