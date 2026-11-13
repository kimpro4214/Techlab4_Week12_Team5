#pragma once

#include "Particles/ParticleEmitterInstance.h"

struct FParticleModuleLifetime
{
	float Lifetime = 2.0f;

	void Spawn(FParticle& Particle) const
	{
		Particle.LifeTime = Lifetime;
	}
};