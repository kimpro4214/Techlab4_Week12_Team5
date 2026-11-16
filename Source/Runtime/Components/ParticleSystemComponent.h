#pragma once

#include "EnginePCH.h"
#include "PrimitiveComponent.h"
#include "Render/Particle/ParticleSystem.h"
#include "Render/Particle/SpriteBatcher.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "ParticleSystemComponent.generated.h"


UCLASS()
class UParticleSystemComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
public:

	UParticleSystemComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
	}

	~UParticleSystemComponent() override { ResetSystem(); }

	void InitializeSystem();	// Template의 Emitter마다 Instance 생성 + Batcher / Bounds 준비
	void ResetSystem();			// Instance / Batcher 해제

	void TickComponent(float deltaTime) override;

	void SetTemplate(UParticleSystem* newTemp)
	{
		if (Template == newTemp) return;
		Template = newTemp;
		if (HasBegunPlay()) InitializeSystem();   // 실행 중이면 바로 다시 만듦
	}

	void BeginPlay() override
	{
		Super::BeginPlay();
		InitializeSystem();
	}

	// 렌더링: 구현은 ParticleSystemComponent.cpp
	void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext) override;	// Emitter마다 Sprite 구간을 채워 패킷 제출
	FBox CalcLocalBounds() const override;	// 컬링용 범위 (기본값은 크기 0 박스라 원점이 화면 밖이면 통째로 컬링됨)

private:
	// 모듈 최댓값(Origin + Location + Velocity x Lifetime + Size/2)으로 BoundsExtent 계산 후 프록시 갱신 요청
	void UpdateBoundsExtent();

	float BoundsExtent = 0.0f;	// 파티클이 퍼질 수 있는 최대 반경 (CalcLocalBounds에서 사용)
	UParticleSystem* Template = nullptr;
	TUniquePtr<FSpriteBatcher> SpriteBatcher = nullptr;
	TArray<FParticleEmitterInstance*> EmitterInstances;
	
};
