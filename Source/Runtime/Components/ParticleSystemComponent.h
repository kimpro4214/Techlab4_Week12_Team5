#pragma once

#include "PrimitiveComponent.h"
#include "Render/Particle/ParticleSystem.h"
#include "Render/Particle/ParticleData.h"   // FParticleCollisionEvent
#include "ParticleSystemComponent.generated.h"

class UParticleSystem;
struct FParticleEmitterInstance;
class FParticleSystemSceneProxy;
class UParticleSystemComponent;

// 파티클 하나가 다른 컴포넌트에 부딪힐 때마다 한 번 호출된다 (어느 파티클 시스템이, 어떤 충돌 정보로)
DECLARE_DELEGATE(FParticleCollideSignature, UParticleSystemComponent*, const FParticleCollisionEvent&);

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
	void SetPreviewEmitterFilter(int32 LODIndex, const UParticleEmitter* SoloEmitter);
	int32 GetEmitterLODIndex() const { return EmitterLODIndex; }
	int32 GetNumActiveParticles() const;

	bool IsCompleted() const { return bHasCompleted; }

	virtual void PostEditChangeProperty(const FProperty& Property) override;

	void InitializeSystem();   // Template의 Emitter마다 Instance를 새로 만든다
	// 시뮬레이션을 진행하지 않고 현재 입자의 렌더 데이터와 뷰 버퍼를 갱신한다.
	void RefreshRenderData();
	void ResetSystem();        // Instance를 모두 지운다

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

	// 바깥(액터, 게임 코드)이 파티클 충돌을 받아 반응할 수 있게 연다
	FParticleCollideSignature OnParticleCollide;

	// 이번 프레임에 모든 이미터에서 난 충돌. 다음 Tick에서 다시 채운다
	const TArray<FParticleCollisionEvent>& GetCollisionEvents() const { return CollisionEvents; }

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

	// 이미터들의 이번 프레임 충돌을 모아 OnParticleCollide로 알린다
	void DispatchCollisionEvents();
	TArray<FParticleCollisionEvent> CollisionEvents;

	bool bPendingDeactivate = false;
	bool bHasCompleted = false;
	bool bSystemInitialized = false;
	// 인스턴스를 만들 때 쓴 Template. 씬 로드처럼 Template만 바뀐 경우를 Tick에서 알아채 다시 만든다
	UParticleSystem* InitializedTemplate = nullptr;

	// 에디터가 소유한 프리뷰 컴포넌트에서만 설정합니다.
	int32 EmitterLODIndex = 0;
	const UParticleEmitter* PreviewSoloEmitter = nullptr;
};
