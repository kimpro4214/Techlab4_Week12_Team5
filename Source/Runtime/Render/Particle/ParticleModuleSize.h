// Render/Particle/Modules/Size/ParticleModuleSize.h
#pragma once
#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleSize.generated.h"

UCLASS()
class UParticleModuleSize : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleSize() { bSpawnModule = true; }

	void Spawn(const FSpawnContext& Context) override;

	UPROPERTY(EditAnywhere) FVector StartSizeMin = FVector(5.0f, 5.0f, 5.0f);
	UPROPERTY(EditAnywhere) FVector StartSizeMax = FVector(10.0f, 10.0f, 10.0f);

	// true면 X만 뽑아서 세 축에 같이 쓴다 (스프라이트가 찌그러지지 않게)
	UPROPERTY(EditAnywhere) bool bUniform = true;
};
