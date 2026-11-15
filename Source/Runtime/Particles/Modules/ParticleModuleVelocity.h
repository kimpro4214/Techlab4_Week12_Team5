#pragma once

#include "Particles/ParticleModule.h"
#include "Math/Vector.h"
#include "ParticleModuleVelocity.generated.h"

UCLASS()
class UParticleModuleVelocity : public UParticleModule
{
	GENERATED_BODY()

public:
	UParticleModuleVelocity();

	virtual bool Validate() const override;
	virtual void Spawn(const FParticleSpawnContext& Context) const noexcept override;

	// 기본 속도에 더할 초기 속도입니다. 생성 행렬이 있으면 회전·스케일을 먼저 적용합니다.
	UPROPERTY(EditAnywhere)
	FVector InitialVelocity = FVector(0.0f);
};
