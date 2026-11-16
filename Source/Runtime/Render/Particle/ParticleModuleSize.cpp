#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleSize.h"
#include "Render/Particle/ParticleEmitterInstance.h"

void UParticleModuleSize::Spawn(const FSpawnContext& Context)
{
	FVector Size = ParticleRandRange(StartSizeMin, StartSizeMax);
	if (bUniform)
		Size = FVector(Size.X, Size.X, Size.X);

	Context.ParticleBase->BaseSize = Context.ParticleBase->Size = Size;
}
