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
	FVector MinSpeed = FVector(-10.0f, -10.0f, 50.0f);
	FVector MaxSpeed = FVector(10.0f, 10.0f, 100.0f);
	bool bUseWorldSpace = false;
	bool bUseOwnerScale = false;
};
