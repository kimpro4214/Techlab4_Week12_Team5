#pragma once

#include "Components/BillboardComponent.h"
#include "Particles/ParticleEmitterInstance.h"
#include "Particles/ParticleSystem.h"
#include "ParticleSystemComponent.generated.h"

UCLASS()
class UParticleSystemComponent : public UBillboardComponent
{
	GENERATED_BODY()

public:
	UParticleSystemComponent();

	void SetParticleSystem(const FParticleSystem& InSystem);
	void BeginPlay() override;
	void TickComponent(float DeltaTime) override;
	FBox CalcLocalBounds() const override;
	FPrimitiveSceneProxy* CreateSceneProxy() override;

private:
	friend class FParticleSystemSceneProxy;

	UPROPERTY(EditAnywhere)
	FParticleSystem ParticleSystem;

	TArray<FParticleEmitterInstance> EmitterInstances;
};
