#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "Render/Material.h"
#include "ParticleModuleRequired.generated.h"

UCLASS()
class UParticleModuleRequired : public UParticleModule
{
	GENERATED_BODY()

private:
	UMaterial* Material = nullptr;		// 출력될 Material
	FVector EmitterLocation;			// Emiiter 위치
	FQuat EmitterRotation;				// Emitter 회전값
	uint32 EmitterLoopCount = 0;		// Emitter 반복 횟수, 0이면 무한 반복
	float EmitterDelay = 0.0f;			// ParticleSystem 시작 후 얼마만큼 Delay를 줄것인가 세팅
	float EmitterDuration = 1.0f;		// Emitter 지속 시간
	float EmitterStartOffset = 0.0f;	// Emitter 시작 시간, 범위 시간일때 StartOffset ~ StartOffset + Duration 시간동안 처리
	bool bKillWhenFinished = false;		// Emitter 동작 종료시 Kill 여부
	bool bKillWhenDisabled = false;		// System 비활성화시 Kill 여부
	bool bUseLocalSpace = false;		// Particle이 Emitter의 Local Space를 따라갈지 여부를 체크, 따라갈경우 Actor를 따라서 Particle도 같이 이동, 따라가지 않으면 Particle은 남음
};
