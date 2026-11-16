// Render/Particle/Modules/Color/ParticleModuleColorOverLife.h
#pragma once
#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleColorOverLife.generated.h"

// 수명(RelativeTime 0→1)에 따라 StartColor → EndColor로 보간한다. 알파를 0으로 두면 페이드아웃
UCLASS()
class UParticleModuleColorOverLife : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleColorOverLife() { bUpdateModule = true; }

	void Update(const FUpdateContext& Context) override;

	UPROPERTY(EditAnywhere) FLinearColor StartColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere) FLinearColor EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);
};
