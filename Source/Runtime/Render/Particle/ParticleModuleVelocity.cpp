#include "EnginePCH.h"
#include "ParticleModuleVelocity.h"
#include "ParticleModuleRequired.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Components/ParticleSystemComponent.h"

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle)
{
	FVector Velocity = FVector(FMath::Lerp(StartVelocityMin.X, StartVelocityMax.X, ParticleRandom()),
		FMath::Lerp(StartVelocityMin.Y, StartVelocityMax.Y, ParticleRandom()), 
		FMath::Lerp(StartVelocityMin.Z, StartVelocityMax.Z, ParticleRandom()));

	bool bLocalSpaceEmitter = Owner->CurrentLODLevel->GetRequiredModule()->IsUseLocalSpace();
	if (!bLocalSpaceEmitter)		// LocalSpace 계산이면 그대로 Velocity 사용. WorldSpace 계산일때만 Velocity를 변환해서 적용
	{
		if (!bUseWorldSpace)  Velocity = Owner->Component->GetWorldMatrix().TransformVectorNoScale(Velocity);   // Velocity의 방향을 월드 기준으로 변경하고, Scale은 적용하지 않음
		if (bUseOwnerScale)  Velocity = Velocity * Owner->Component->GetWorldScale3D(); // 스케일은 이 옵션으로만
	}

	OutParticle->Velocity = Velocity;
	OutParticle->BaseVelocity = OutParticle->Velocity;
}
