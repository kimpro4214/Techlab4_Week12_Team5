#include "EnginePCH.h"
#include "ParticleEmitterInstance.h"
#include "Particles/ParticleModuleSpawn.h"

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

int32 FParticleEmitterInstance::ConsumeSpawnCount(const FParticleModuleSpawn& SpawnModule, float DeltaTime)
{
	return SpawnModule.GetSpawnCount(DeltaTime, SpawnRemainder);
}
