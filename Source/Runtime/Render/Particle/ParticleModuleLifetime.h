#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleModuleLifetime.generated.h"

UCLASS()
class UParticleModuleLifetime : public UParticleModule
{
	GENERATED_BODY()
public:
	void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle) override;
private:
	float MinLifeTime = 1.0f;	// 수명 최소 시간
	float MaxLifeTime = 1.0f;	// 수명 최대 시간
};
