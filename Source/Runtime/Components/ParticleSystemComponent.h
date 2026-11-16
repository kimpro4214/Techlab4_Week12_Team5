#pragma once

#include "PrimitiveComponent.h"
#include "ParticleSystemComponent.generated.h"

class UParticleSystem;               // 정의: Render/Particle/ParticleSystem.h
struct FParticleEmitterInstance;     // 정의: Render/Particle/ParticleEmitterInstance.h
struct FDynamicEmitterDataBase;      // 정의: Render/Particle/ParticleDynamicData.h

// 월드에 배치되는 파티클 (UE: UParticleSystemComponent)
// Template의 Emitter마다 EmitterInstance를 만들어 돌린다.
UCLASS()
class UParticleSystemComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
public:
	UParticleSystemComponent();
	virtual ~UParticleSystemComponent() override;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;
	virtual FBox CalcLocalBounds() const override;
	virtual void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext) override;

	void SetTemplate(UParticleSystem* NewTemplate);
	UParticleSystem* GetTemplate() const { return Template; }

	void InitializeSystem();   // Template의 Emitter마다 Instance를 새로 만든다
	void ResetSystem();        // Instance를 모두 지운다

	int32 GetNumActiveParticles() const;   // 모든 Emitter의 활성 파티클 합 (Stat/디버그용)

private:
	UParticleSystem* Template = nullptr;
	TArray<FParticleEmitterInstance*> EmitterInstances;   // 이 컴포넌트가 소유

	// 이번 프레임에 그릴 이미터 데이터. Tick 끝에서 프레임당 한 번 모은다
	// 소유권은 각 EmitterInstance에 있다 → 인스턴스를 지울 때 같이 비워야 한다
	TArray<FDynamicEmitterDataBase*> EmitterRenderData;

	// 파티클은 월드 공간에서 날아가므로 컬링 박스를 넉넉하게 잡는다
	float BoundsExtent = 10.0f;

	float DebugLogTimer = 0.0f;   // [임시] 1초마다 활성 수 로그. 렌더가 붙으면 지운다
};
