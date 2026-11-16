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

	void InitializeSystem()	// EmitterInstance 초기 생성
	{
		ResetSystem();
		if (!Template) return;
		int32 MaxParticleCount = 0;
		for (UParticleEmitter* Emitter : Template->GetEmitterArray())
		{
			FParticleEmitterInstance* EmitterInstance = new FParticleEmitterInstance();
			if (EmitterInstance->Init(Emitter, this))
			{EmitterInstances.Add(EmitterInstance);
			MaxParticleCount += EmitterInstance->MaxActiveParticles;
			}
			else delete EmitterInstance;
		}
		if (MaxParticleCount > 0) {
			SpriteBatcher = MakeUnique<FSpriteBatcher>();
			SpriteBatcher.get()->Init(MaxParticleCount);		// PSC당 Batcher 하나 사용, 전체 파티클 세팅
		}
	}

	void ResetSystem()	// EmitterInstnace 리셋
	{
		for (FParticleEmitterInstance* Inst : EmitterInstances)
		{
			delete Inst;
		}
		EmitterInstances.Empty();
		SpriteBatcher.reset();
	}

	void TickComponent(float deltaTime) override
	{
		for (FParticleEmitterInstance* Inst : EmitterInstances)
		{
			Inst->Update(deltaTime);
			Inst->Spawn(deltaTime);
		}
	}

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
	UParticleSystem* Template = nullptr;
	TUniquePtr<FSpriteBatcher> SpriteBatcher = nullptr;
	TArray<FParticleEmitterInstance*> EmitterInstances;
	
};
