#pragma once

#include "EnginePCH.h"
#include "PrimitiveComponent.h"
#include "ParticleSystemComponent.generated.h"

class UParticleSystem;

UCLASS()
class UParticleSystemComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
public:

	
private:
	UParticleSystem* Template;
	TArray<FParticleEmitterInstance*> EmitterInstances;
};
