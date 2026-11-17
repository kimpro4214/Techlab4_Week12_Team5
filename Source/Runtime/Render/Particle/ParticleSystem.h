#pragma once

#include "Asset/Asset.h"
#include "Render/Particle/ParticleEmitter.h"
#include "UObject/Class.h"
#include "ParticleSystem.generated.h"

class UParticleEmitter;

// 파티클 에셋 . Emitter 여러 개를 동시에 재생한다.
UCLASS()
class UParticleSystem : public UAsset
{
	GENERATED_BODY()
public:
	static UParticleSystem* CreateDefault();

	const TArray<UParticleEmitter*>& GetEmitters() const { return Emitters; }
	void AddEmitter(UParticleEmitter* InEmitter);

	virtual bool ValidateAsset(FString& OutError) const override;
	virtual void PostLoadAsset() override;

	float GetLOD1Distance() const { return LOD1Distance; }

private:
	UPROPERTY()
	TArray<UParticleEmitter*> Emitters;

	UPROPERTY()
	float LOD1Distance = 6.0f;
};
