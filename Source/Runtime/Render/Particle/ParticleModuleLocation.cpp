#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleLocation.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "ParticleModuleRequired.h"
#include "Components/ParticleSystemComponent.h"

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle)
{
	FVector LocationOffset = FVector(FMath::Lerp(MinLocation.X, MaxLocation.X, ParticleRandom()),
		FMath::Lerp(MinLocation.Y, MaxLocation.Y, ParticleRandom()),
		FMath::Lerp(MinLocation.Z, MaxLocation.Z, ParticleRandom()));

	bool bLocalSpaceEmitter = Owner->CurrentLODLevel->GetRequiredModule()->IsUseLocalSpace();
	if (!bLocalSpaceEmitter)		// LocalSpace 계산이면 그대로 Location 사용. WorldSpace 계산일때만 Location를 변환해서 적용
	{
		LocationOffset = Owner->Component->GetWorldMatrix().TransformVector(LocationOffset);
	}

	OutParticle->Location += LocationOffset;
}
