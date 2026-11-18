#pragma once

#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleSizeMultiplyLife.generated.h"

class UCurveFloat;

UCLASS()
class UParticleModuleSizeMultiplyLife : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleSizeMultiplyLife()
	{
		bSpawnModule = true;
		bUpdateModule = true;
	}

	void Spawn(const FSpawnContext& Context) override;
	void Update(const FUpdateContext& Context) override;

	UCurveFloat* SizeScaleCurve = nullptr;

private:
	float GetSizeScale(float Age) const;
};