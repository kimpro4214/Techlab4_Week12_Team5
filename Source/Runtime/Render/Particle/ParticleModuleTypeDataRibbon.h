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

	// 마디 사이를 3차(Hermite) 곡선으로 메울 중간 점 수를 정한다. 세 값 중 하나라도 0 이하면 직선으로 잇는다
	// 직선 구간은 나눠도 모양이 같으므로, 꺾인 구간에만 점을 넣는다
	UPROPERTY(EditAnywhere) float DistanceTessellationStepSize    = 0.1f;   // 꺾인 구간에서 이 길이마다 중간 점 하나
	UPROPERTY(EditAnywhere) float TangentTessellationScalar       = 8.0f;   // 꺾인 정도(0 ~ 2)에 곱해 중간 점 수를 정한다
	UPROPERTY(EditAnywhere) int32 MaxTessellationBetweenParticles = 16;     // 마디 사이 중간 점 최대 개수

	// 곡선의 접선(휘는 방향과 세기)은 마디가 태어날 때 소스가 움직인 속도(위치 차 ÷ 시간 차)로 정한다
	// 이 옵션을 켜면 태어날 때 값으로 고정하지 않고, 매 프레임 이웃 마디의 위치·시각으로 다시 계산한다
	// 마디가 태어난 뒤에도 움직이는(속도·가속도가 있는) 리본에서 켠다
	UPROPERTY(EditAnywhere) bool bTangentRecalculationEveryFrame = false;
};
