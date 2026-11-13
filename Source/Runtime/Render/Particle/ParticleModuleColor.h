#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "Math/Color.h"
#include "ParticleModuleColor.generated.h"

UCLASS()
class UParticleModuleColor : public UParticleModule
{
	GENERATED_BODY()
public:
	void Spawn() override;
	void Update() override;

private:
	FLinearColor StartColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);	// 생성 시점 색
	FLinearColor EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);	// 수명 종료 시점 색 (기본: 점점 투명해짐)
};
