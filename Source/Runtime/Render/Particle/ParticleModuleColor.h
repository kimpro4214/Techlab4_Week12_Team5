#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleModuleColor.generated.h"

UCLASS()
class UParticleModuleColor : public UParticleModule
{
	GENERATED_BODY()
public:
	void Spawn() override;
	void Update() override;

private:
	FVector Color = FVector(0,0,0);
	float Alpha = 1;
};
