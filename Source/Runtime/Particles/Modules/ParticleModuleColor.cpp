#include "EnginePCH.h"
#include "ParticleModuleColor.h"
#include "Particles/BaseParticle.h"


UParticleModuleColor::UParticleModuleColor()
{
	bSpawnModule = true;
}

bool UParticleModuleColor::Validate() const
{
	return true;
}

void UParticleModuleColor::Spawn(const FParticleSpawnContext& Context) const noexcept
{
	Context.Particle.BaseColor = InitialColor;
	Context.Particle.Color = InitialColor;
}
