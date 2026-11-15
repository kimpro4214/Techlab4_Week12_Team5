#pragma once

#include "Particles/ParticleModule.h"

#include "ParticleModuleLifetime.generated.h"

UCLASS()
class UParticleModuleLifetime : public UParticleModule
{
	GENERATED_BODY()

public:
	UParticleModuleLifetime();

	virtual bool Validate() const override;
	virtual void Spawn(const FParticleSpawnContext& Context) const noexcept override;

	// 생성 시 적용하는 상수 설정입니다. 실행 중 변경은 Instance 재초기화로 반영합니다.
	UPROPERTY(EditAnywhere)
	float Lifetime = 1.0f;
};
