#pragma once

#include "PrimitiveComponent.h"
#include "Containers/Array.h"
#include "Core/Types.h"
#include "ParticleSystemComponent.generated.h"

class UParticleSystem;
struct FParticleEmitterInstance;
struct FDynamicEmitterDataBase;

UCLASS()
class UParticleSystemComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	TArray<FParticleEmitterInstance*> EmitterInstances;
	UParticleSystem* Template;

	TArray<FDynamicEmitterDataBase*> EmitterRenderData;
};
