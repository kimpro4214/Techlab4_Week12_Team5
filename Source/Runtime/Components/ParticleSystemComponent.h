#pragma once

#include "PrimitiveComponent.h"
#include "Containers/Array.h"
#include "Core/Types.h"

class UParticleSystem;
struct FParticleEmitterInstance;
struct FDynamicEmitterDataBase;

UCLASS()
class UParticleSystemComponent : public UPrimitiveComponent
{
	TArray<FParticleEmitterInstance*> EmitterInstances;
	UParticleSystem* Template;

	TArray<FDynamicEmitterDataBase*> EmitterRenderData;
};
