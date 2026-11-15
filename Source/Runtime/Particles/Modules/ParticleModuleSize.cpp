#include "EnginePCH.h"
#include "ParticleModuleSize.h"
#include "Particles/BaseParticle.h"


UParticleModuleSize::UParticleModuleSize()
{
	bSpawnModule = true;
}

bool UParticleModuleSize::Validate() const
{
	return IsFiniteVector(InitialSize) &&
		InitialSize.X >= 0.0f && InitialSize.Y >= 0.0f && InitialSize.Z >= 0.0f;
}

void UParticleModuleSize::Spawn(const FParticleSpawnContext& Context) const noexcept
{
	Context.Particle.BaseSize = InitialSize;
	Context.Particle.Size = InitialSize;
}
