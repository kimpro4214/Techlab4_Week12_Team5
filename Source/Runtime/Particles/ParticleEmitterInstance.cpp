#include "EnginePCH.h"
#include "ParticleEmitterInstance.h"

#include <algorithm>

void FParticleEmitterInstance::Reset()
{
	Particles.Reset();
	SpawnRemainder = 0.0f;
}

void FParticleEmitterInstance::UpdateParticles(float DeltaTime)
{
	for (int32 Index = Particles.Num() - 1; Index >= 0; --Index)
	{
		FParticle& Particle = Particles[Index];
		Particle.Location += Particle.Velocity * DeltaTime;
		Particle.Age += DeltaTime;

		if (Particle.Age >= Particle.LifeTime)
		{
			Particles.RemoveAtSwap(Index);
		}
	}
}

int32 FParticleEmitterInstance::ConsumeSpawnCount(float SpawnRate, float DeltaTime)
{
	SpawnRemainder += std::max(0.0f, SpawnRate) * DeltaTime;

	const int32 SpawnCount = static_cast<int32>(SpawnRemainder);
	SpawnRemainder -= static_cast<float>(SpawnCount);
	return SpawnCount;
}
