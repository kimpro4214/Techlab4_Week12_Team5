#include "EnginePCH.h"
#include "ParticleModuleLocation.h"
#include "Particles/BaseParticle.h"
#include "Math/Matrix.h"

UParticleModuleLocation::UParticleModuleLocation()
{
	bSpawnModule = true;
}

bool UParticleModuleLocation::Validate() const
{
	return IsFiniteVector(InitialLocation);
}

void UParticleModuleLocation::Spawn(const FParticleSpawnContext& Context) const noexcept
{
	const FVector Offset = Context.EmitterToWorld
		? Context.EmitterToWorld->TransformVector(InitialLocation) : InitialLocation;
	Context.Particle.Location += Offset;
}
