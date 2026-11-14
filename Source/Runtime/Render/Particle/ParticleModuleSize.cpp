#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleSize.h"
#include "Render/Particle/ParticleEmitterInstance.h"

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle)
{
	OutParticle->Size = FMath::Lerp(MinSize, MaxSize, ParticleRandom());
}
