#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleModuleVelocity.generated.h"

UCLASS()
class UParticleModuleVelocity : public UParticleModule
{
	GENERATED_BODY()

public:
	void Spawn() override;

private:

	// 초기 속도(방향 + 크기 벡터). 축마다 [Min, Max]에서 랜덤으로 결정
	FVector StartVelocityMin = FVector(-10.0f, -10.0f, 50.0f);		// Particle 초기 속도 하한
	FVector StartVelocityMax = FVector(10.0f, 10.0f, 100.0f);		// Particle 초기 속도 상한

	bool bUseWorldSpace = false;	// 주어전 Speed를 월드축 속도로 처리, 체크 해제시 로컬축 속도로 처리
	bool bUseOwnerScale = false;	// ParticleSystemComponent의 Scale 값을 속도에 반영
};
