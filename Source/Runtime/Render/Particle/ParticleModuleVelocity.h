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
	FVector MinSpeed = FVector(-10.0f, -10.0f, 50.0f);		// Particle 초기 최소 속도
	FVector MaxSpeed = FVector(10.0f, 10.0f, 100.0f);		// Particle 초기 최대 속도
	bool bUseWorldSpace = false;	// 주어전 Speed를 월드 기준 속도로 처리, 체크 해제시 로컬 기준 속도로 처리					
	bool bUseOwnerScale = false;	// ParticleSystemComponent의 Scale 값을 속도에 반영
};
