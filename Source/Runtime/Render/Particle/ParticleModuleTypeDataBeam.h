#pragma once

#include "Render/Particle/ParticleModuleTypeDataBase.h"
#include "ParticleModuleTypeDataBeam.generated.h"

// 두 점(Source → Target)을 잇는 띠를 그린다
// 점을 만드는 규칙(소스·타깃 지정, 노이즈 등)은 시뮬레이션 담당이 채운다
// 지금은 렌더링 확인용으로 이미터 원점에서 TargetOffset까지 직선을 나눠 점을 만든다
UCLASS()
class UParticleModuleTypeDataBeam : public UParticleModuleTypeDataBase
{
	GENERATED_BODY()
public:
	FParticleEmitterInstance* CreateInstance() override;

	UPROPERTY(EditAnywhere) FVector      TargetOffset = FVector(0.0f, 0.0f, 3.0f);   // 컴포넌트 로컬 기준 끝점
	UPROPERTY(EditAnywhere) int32        SegmentCount = 16;                         // 구간 수 (점 수 = 구간 수 + 1)
	UPROPERTY(EditAnywhere) float        Width        = 0.3f;
	UPROPERTY(EditAnywhere) FLinearColor Color        = FLinearColor(0.4f, 0.7f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere) float        UVTiling     = 0.0f;   // 0 이하: 전체 길이에 텍스처 하나
	UPROPERTY(EditAnywhere) float        NoiseAmplitude = 0.0f; // 중간 점이 x/y/z로 흔들리는 최대 거리(월드 단위). 0이면 직선
};
