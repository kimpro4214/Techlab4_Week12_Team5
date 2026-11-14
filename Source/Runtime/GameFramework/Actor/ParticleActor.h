#pragma once

#include "GameFramework/Actor.h"
#include "Components/ParticleSystemComponent.h"
#include "ParticleActor.generated.h"

UCLASS()
class AParticleActor : public AActor
{
	GENERATED_BODY()


public:
	AParticleActor();

private:
	UParticleSystemComponent* ParticleComponent = nullptr;
};
