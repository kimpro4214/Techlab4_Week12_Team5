#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleModuleSpawn.generated.h"

UCLASS()
class UParticleModuleSpawn : public UParticleModule
{
	GENERATED_BODY()
private:
	float Rate = 20.0;	// 생성 속도 
	float RateScale = 1.0;	// 속도 스케일, Speed * SpeedScale => 초당 생성하는 파티클 개수
};
