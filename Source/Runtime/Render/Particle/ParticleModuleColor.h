#pragma once
#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleColor.generated.h"

UCLASS()
class UParticleModuleColor : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleColor() { bSpawnModule = true; }

	void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
	{
		ParticleBase->BaseColor = ParticleBase->Color = StartColor;
	}

	UPROPERTY(EditAnywhere) FLinearColor StartColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};