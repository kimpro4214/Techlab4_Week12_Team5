#pragma once

#include "EnginePCH.h"
#include "ParticleEmitter.h"
#include "ParticleSystem.generated.h"

UCLASS()
class UParticleSystem : public UObject
{
	GENERATED_BODY()
public:
	// 값 반환이면 호출마다 배열 전체가 복사되므로 const 참조로 반환
	const TArray<UParticleEmitter*>& GetEmitterArray() const { return Emitters; }
	void AddEmitter(UParticleEmitter* InEmitter) { if (InEmitter) Emitters.Add(InEmitter); }

private:
	TArray<UParticleEmitter*> Emitters;
};
