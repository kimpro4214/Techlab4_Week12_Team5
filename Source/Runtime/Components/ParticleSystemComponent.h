#pragma once

#include "EnginePCH.h"
#include "PrimitiveComponent.h"
#include "Render/Particle/ParticleSystem.h"
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
		for (UParticleEmitter* Emitter : Template->GetEmitterArray())
		{
			FParticleEmitterInstance* EmitterInstance = new FParticleEmitterInstance();
			if (EmitterInstance->Init(Emitter, this)) EmitterInstances.Add(EmitterInstance);
			else delete EmitterInstance;
		}
	}

	void ResetSystem()	// EmitterInstnace 리셋
	{
		for (FParticleEmitterInstance* Inst : EmitterInstances)
		{
			delete Inst;
		}
		EmitterInstances.Empty();
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

private:
	UParticleSystem* Template = nullptr;
	TArray<FParticleEmitterInstance*> EmitterInstances;
};
