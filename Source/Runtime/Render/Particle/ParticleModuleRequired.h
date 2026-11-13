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
	FVector EmitterOrigin;				// Emitter 위치
	FRotator EmitterRotation;			// Emitter 회전값
	uint32 EmitterLoopCount = 0;		// Emitter 반복 횟수, 0이면 무한 반복

	// 지속 시간: bEmitterDurationUseRange가 false면 EmitterDuration 고정,
	// true면 루프 시작 시 [EmitterDurationLow, EmitterDuration]에서 랜덤으로 결정
	float EmitterDuration = 1.0f;				// Emitter 지속 시간 (범위 사용 시 상한)
	float EmitterDurationLow = 0.0f;			// 범위 사용 시 지속 시간 하한
	bool bEmitterDurationUseRange = false;		// 지속 시간을 범위 랜덤으로 결정할지 여부

	float EmitterDelay = 0.0f;			// Emitter 활성화 후 첫 파티클 생성까지 기다리는 시간(초)
	bool bKillWhenFinished = false;		// Emitter 동작 종료시 Kill 여부
	bool bKillWhenDisabled = false;		// System 비활성화시 Kill 여부
	bool bUseLocalSpace = false;		// Particle이 Emitter의 Local Space를 따라갈지 여부를 체크, 따라갈경우 Actor를 따라서 Particle도 같이 이동, 따라가지 않으면 Particle은 남음
};
