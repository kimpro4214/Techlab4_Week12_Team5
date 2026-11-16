#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleSize.h"
#include "Render/Particle/ParticleEmitterInstance.h"

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	FVector Size = ParticleRandRange(StartSizeMin, StartSizeMax);
	if (bUniform)
		Size = FVector(Size.X, Size.X, Size.X);

	ParticleBase->BaseSize = ParticleBase->Size = Size;
}
