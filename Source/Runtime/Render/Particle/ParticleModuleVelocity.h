#pragma once
#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleVelocity.generated.h"

UCLASS()
class UParticleModuleVelocity : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleVelocity() { bSpawnModule = true; }

	void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

	UPROPERTY(EditAnywhere) FVector StartVelocityMin = FVector(-20.0f, -20.0f, 50.0f);
	UPROPERTY(EditAnywhere) FVector StartVelocityMax = FVector(20.0f, 20.0f, 100.0f);

	// true : 입력값을 월드 축 기준으로 쓴다 (액터를 돌려도 항상 위로 솟음)
	// false: 이미터 로컬 축 기준 (액터 회전을 따라감)
	UPROPERTY(EditAnywhere) bool bInWorldSpace = false;
};