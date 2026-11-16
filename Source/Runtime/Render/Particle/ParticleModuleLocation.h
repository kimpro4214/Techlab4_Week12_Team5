// Render/Particle/Modules/Location/ParticleModuleLocation.h
#pragma once
#include "Render/Particle/ParticleModule.h"
#include "ParticleModuleLocation.generated.h"

// 생성 위치에 박스 범위 랜덤 오프셋을 더한다
UCLASS()
class UParticleModuleLocation : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleLocation() { bSpawnModule = true; }

	void Spawn(const FSpawnContext& Context) override;

	UPROPERTY(EditAnywhere) FVector StartLocationMin = FVector(-10.0f, -10.0f, 0.0f);   // 이미터 로컬 기준
	UPROPERTY(EditAnywhere) FVector StartLocationMax = FVector(10.0f, 10.0f, 0.0f);
};
