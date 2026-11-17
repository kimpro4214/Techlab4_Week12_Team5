#include "EnginePCH.h"
#include "Engine/ParticleSystemSceneProxy.h"

#include "Components/ParticleSystemComponent.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Render/RenderPacket.h"

FParticleSystemSceneProxy::FParticleSystemSceneProxy(UParticleSystemComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
}

void FParticleSystemSceneProxy::GatherMeshElements(FRenderQueue& Queue, const FViewContext& View) const
{
	if (!IsShown(View))
		return;

	// Submit은 뷰별 VB를 하나씩 꺼내 쓰므로 const가 아니다. 포인터가 가리키는 대상이라 여기서 호출할 수 있다
	for (FDynamicEmitterDataBase* Data : DynamicData)
		Data->Submit(Queue, View);
}
