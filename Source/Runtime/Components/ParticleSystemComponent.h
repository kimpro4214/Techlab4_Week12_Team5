#pragma once

#include "PrimitiveComponent.h"
#include "Render/Particle/ParticleSystem.h"
#include "ParticleSystemComponent.generated.h"

class UParticleSystem;               
struct FParticleEmitterInstance;     
class FParticleSystemSceneProxy;     

// 월드에 배치되는 파티클 
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
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	void SetTemplate(UParticleSystem* NewTemplate);
	UParticleSystem* GetTemplate() const { return Template; }

	bool IsCompleted() const { return bHasCompleted; }

	virtual void PostEditChangeProperty(const FProperty& Property) override;

	void InitializeSystem();   // Template의 Emitter마다 Instance를 새로 만든다
	void ResetSystem();        // Instance를 모두 지운다

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* Template = nullptr;
	TArray<FParticleEmitterInstance*> EmitterInstances;   // 이 컴포넌트가 소유

	// 이번 프레임 렌더 데이터를 모아 Proxy에 넘긴다 
	void SendRenderDynamicData();
	FParticleSystemSceneProxy* GetParticleSceneProxy() const;

	// 이미터들이 그리는 것을 모두 감싸도록 LocalBounds를 갱신하고, 바뀌면 컬링(BVH)에 반영한다
	void UpdateComponentBounds();

	// 컴포넌트 로컬 공간 Bounds. 첫 파티클이 나오기 전에는 원점 주변 작은 박스
	FBox LocalBounds = FBox(FVector(-1.0f, -1.0f, -1.0f), FVector(1.0f, 1.0f, 1.0f));

	bool bPendingDeactivate = false;
	bool bHasCompleted = false;
};
