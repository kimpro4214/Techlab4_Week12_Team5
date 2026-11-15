#include "EnginePCH.h"
#include "ParticleModuleVelocity.h"
#include "Particles/BaseParticle.h"
#include "Math/Matrix.h"

UParticleModuleVelocity::UParticleModuleVelocity()
{
	bSpawnModule = true;
}

bool UParticleModuleVelocity::Validate() const
{
	return IsFiniteVector(InitialVelocity);
}

void UParticleModuleVelocity::Spawn(const FParticleSpawnContext& Context) const noexcept
{
	const FVector Velocity = Context.EmitterToWorld
		? Context.EmitterToWorld->TransformVector(InitialVelocity) : InitialVelocity;
	Context.Particle.BaseVelocity += Velocity;
	Context.Particle.Velocity += Velocity;
}
