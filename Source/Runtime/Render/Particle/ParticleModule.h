#pragma once

#include "EnginePCH.h"
#include "ParticleModule.generated.h"

UCLASS()
class UParticleModule : public UObject
{
	GENERATED_BODY()
public:
	virtual void Spawn() {}

	virtual void Update() {}
};
