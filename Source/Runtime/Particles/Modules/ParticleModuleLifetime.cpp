#include "EnginePCH.h"
#include "ParticleModuleLifetime.h"
#include "Particles/BaseParticle.h"
#include <cmath>

UParticleModuleLifetime::UParticleModuleLifetime()
{
	bSpawnModule = true;
}

bool UParticleModuleLifetime::Validate() const
{
	return std::isfinite(Lifetime) && Lifetime > 0.0f;
}

void UParticleModuleLifetime::Spawn(const FParticleSpawnContext& Context) const noexcept
{
	Context.Particle.Lifetime = Lifetime;
}
