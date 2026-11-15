#pragma once

#include "GameFramework/Actor.h"
#include "Components/ParticleSystemComponent.h"
#include "ParticleSystemActor.generated.h"

// 파티클 시스템을 월드에 배치하는 액터 
UCLASS()
class AParticleSystemActor : public AActor
{
	GENERATED_BODY()
public:
	AParticleSystemActor();

	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

private:
	UParticleSystemComponent* ParticleSystemComponent = nullptr;
};
