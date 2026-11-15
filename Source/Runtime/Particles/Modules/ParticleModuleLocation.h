#pragma once

#include "Particles/ParticleModule.h"
#include "Math/Vector.h"
#include "ParticleModuleLocation.generated.h"

UCLASS()
class UParticleModuleLocation : public UParticleModule
{
	GENERATED_BODY()

public:
	UParticleModuleLocation();

	virtual bool Validate() const override;
	virtual void Spawn(const FParticleSpawnContext& Context) const noexcept override;

	// 생성 위치에 더할 오프셋입니다. 생성 행렬이 있으면 회전·스케일을 먼저 적용합니다.
	UPROPERTY(EditAnywhere)
	FVector InitialLocation = FVector(0.0f);
};
