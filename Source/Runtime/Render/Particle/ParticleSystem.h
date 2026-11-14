#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleSystem.generated.h"

class UParticleEmitter;

// 파티클 에셋 . Emitter 여러 개를 동시에 재생한다.
UCLASS()
class UParticleSystem : public UObject
{
	GENERATED_BODY()
public:
	const TArray<UParticleEmitter*>& GetEmitters() const { return Emitters; }
	void AddEmitter(UParticleEmitter* InEmitter) { if (InEmitter) Emitters.Add(InEmitter); }

private:
	TArray<UParticleEmitter*> Emitters;
};
