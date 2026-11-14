#include "EnginePCH.h"
#include "ParticleEmitterInstance.h"
#include "Particles/ParticleEmitter.h"

#include <algorithm>

void FParticleEmitterInstance::Reset()
{
	Particles.Reset();
	SpawnRemainder = 0.0f;
	EmitterTime = 0.0f;
}

void FParticleEmitterInstance::UpdateParticles(float DeltaTime)
{
	for (int32 Index = Particles.Num() - 1; Index >= 0; --Index)
	{
		FBaseParticle& Particle = Particles[Index];
		Particle.Location += Particle.Velocity * DeltaTime;
		Particle.Age += DeltaTime;

		if (Particle.Age >= Particle.LifeTime)
		{
			Particles.RemoveAtSwap(Index);
		}
	}
}

int32 FParticleEmitterInstance::ConsumeSpawnCount(const FParticleEmitter& Emitter, float DeltaTime)
{
	float SpawnDeltaTime = DeltaTime;
	if (Emitter.Required.Duration > 0.0f)
	{
		const float RemainingTime = Emitter.Required.Duration - EmitterTime;
		SpawnDeltaTime = std::min(DeltaTime, std::max(0.0f, RemainingTime));
	}

	EmitterTime += DeltaTime;
	return Emitter.Spawn.GetSpawnCount(SpawnDeltaTime, SpawnRemainder);
}
