#pragma once

#include "Asset/Asset.h"
#include "Render/Particle/ParticleEmitter.h"
#include "UObject/Class.h"
#include "Math/Box.h"
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

	// 호출 전에 두 시스템을 사용하는 실행 인스턴스를 해제해야 합니다.
	// 이미터의 소유권을 함께 교환하여 공유 에셋 객체의 주소를 유지합니다.
	void SwapEmitters(UParticleSystem& Other);

	virtual bool ValidateAsset(FString& OutError) const override;
	virtual void PostLoadAsset() override;

	float GetLOD1Distance() const { return LOD1Distance; }

	bool IsUseFixedRelativeBoundingBox() const { return bUseFixedRelativeBoundingBox; }
	const FBox& GetFixedRelativeBoundingBox() const { return FixedRelativeBoundingBox; }
	void SetUseFixedRelativeBoundingBox(bool bInUse) { bUseFixedRelativeBoundingBox = bInUse; }
	void SetFixedRelativeBoundingBox(const FBox& InBox) { FixedRelativeBoundingBox = InBox; }

private:
	UPROPERTY()
	TArray<UParticleEmitter*> Emitters;

	UPROPERTY()
	float LOD1Distance = 6.0f;

	// true면 매 프레임 파티클로 Bounds를 계산하지 않고 아래 박스(컴포넌트 로컬)를 쓴다
	bool bUseFixedRelativeBoundingBox = false;
	FBox FixedRelativeBoundingBox = FBox(FVector(-5.0f, -5.0f, -5.0f), FVector(5.0f, 5.0f, 5.0f));
};
