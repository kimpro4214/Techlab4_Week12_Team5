#pragma once

#include "Engine/PrimitiveSceneProxy.h"

class UParticleSystemComponent;
struct FDynamicEmitterDataBase;

// 파티클 시스템의 렌더 쪽 대리자
// 컴포넌트(게임)는 시뮬레이션만 하고, 렌더러는 이 Proxy만 본다.
// 싱글 스레드라 DynamicData를 복사하지 않고 포인터만 넘겨받는다 (소유는 각 EmitterInstance).
class FParticleSystemSceneProxy : public FPrimitiveSceneProxy
{
public:
	explicit FParticleSystemSceneProxy(UParticleSystemComponent* InComponent);

	// Show Flag(Particles)가 꺼지면 그리지 않는다. 시뮬레이션은 컴포넌트에서 계속 돈다
	bool IsShown(const FViewContext& View) const override;

	// PSC가 Tick 끝에서 이번 프레임 렌더 데이터를 넘긴다
	void ResetDynamicData() { DynamicData.Reset(); }
	void AddDynamicData(FDynamicEmitterDataBase* Data) { DynamicData.Add(Data); }

	// 뷰마다 불린다. 이미터 하나 = 패킷 하나
	void GatherMeshElements(FRenderQueue& Queue, const FViewContext& View) const override;

private:
	TArray<FDynamicEmitterDataBase*> DynamicData;
};
