#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleLifetime.h"
#include "Render/Particle/ParticleEmitterInstance.h"

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle)
{
	float Life = FMath::Lerp(MinLifeTime, MaxLifeTime, ParticleRandom());
	OutParticle->Lifetime = (Life > 0.0f) ? 1.0f / Life : 0.0f; 	// RelativeTime 계산 편의성을 위해 미리 나눔
}
