#pragma once

#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleCollision.generated.h"

UCLASS()
class UParticleModuleCollision : public UParticleModule
{
	GENERATED_BODY()

public:
	UParticleModuleCollision()
	{
		bSpawnModule = true;
		bUpdateModule = true;
	}

	void Spawn(const FSpawnContext& Context) override;
	void Update(const FUpdateContext& Context) override;

	// 반사된 속도에서 유지할 비율. 0은 정지, 1은 감쇠 없는 반사다.
	UPROPERTY(EditAnywhere, meta=(ClampMin=0, ClampMax=1, Delta=0.01))
	float DampingFactor = 0.6f;
};
