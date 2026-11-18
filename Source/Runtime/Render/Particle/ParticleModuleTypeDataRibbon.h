#pragma once

#include "Render/Particle/ParticleModuleTypeDataBase.h"
#include "ParticleModuleTypeDataRibbon.generated.h"

// 파티클을 생성 순서대로 이어 궤적(띠)을 그린다
// 띠의 폭·색·수명은 기존 Size / Color / Lifetime 모듈을 그대로 쓴다 (파티클 하나 = 띠 위의 점 하나)
UCLASS()
class UParticleModuleTypeDataRibbon : public UParticleModuleTypeDataBase
{
	GENERATED_BODY()
public:
	FParticleEmitterInstance* CreateInstance() override;

	UPROPERTY(EditAnywhere) float UVTiling = 0.0f;   // 0 이하: 전체 길이에 텍스처 하나
};
