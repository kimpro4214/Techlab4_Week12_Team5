#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleSystem.generated.h"

class UParticleEmitter;

UCLASS()
class UParticleSystem : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UParticleSystem() override;

	bool Validate() const;
	bool Build();

	// 새 Emitter를 추가합니다.
	UParticleEmitter* AddEmitter();
	UParticleEmitter* GetEmitter(int32 Index) const;

	// 모든 Emitter를 반환합니다.
	const TArray<UParticleEmitter*>& GetEmitters() const
	{
		return Emitters;
	}

private:
	TArray<UParticleEmitter*> Emitters;
};
