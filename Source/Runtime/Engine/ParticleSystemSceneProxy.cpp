#include "EnginePCH.h"
#include "Engine/ParticleSystemSceneProxy.h"

#include "Components/ParticleSystemComponent.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Render/RenderPacket.h"
#include "Render/SceneRenderData.h"
#include "Render/Particle/ParticleStats.h"
#include <algorithm>

FParticleSystemSceneProxy::FParticleSystemSceneProxy(UParticleSystemComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
}

bool FParticleSystemSceneProxy::IsShown(const FViewContext& View) const
{
	return View.bShowParticles && FPrimitiveSceneProxy::IsShown(View);
}

void FParticleSystemSceneProxy::GatherMeshElements(FRenderQueue& Queue, const FViewContext& View) const
{
	if (!IsShown(View))
		return;

	SCOPE_CYCLE_COUNTER(ParticleStats::STAT_ParticleSubmit);

	bool bDrewAny = false;
	for (FDynamicEmitterDataBase* Data : DynamicData)
	{
		// Submit은 뷰별 VB를 하나씩 꺼내 쓰므로 const가 아니다. 포인터가 가리키는 대상이라 여기서 호출할 수 있다
		const int32 FirstPacket = Queue.Num();
		Data->Submit(Queue, View);

		const int32 NewPackets = Queue.Num() - FirstPacket;
		if (NewPackets <= 0 || !View.Stats)
			continue;

		// 이 이미터가 이번 뷰에 낸 패킷만큼 통계에 더한다
		bDrewAny = true;
		FRenderStats& Stats = *View.Stats;
		++Stats.ParticleEmitters;
		Stats.ParticlesRendered += static_cast<uint32>(std::max(0, Data->GetSource().ActiveParticleCount));
		Stats.ParticleDrawCalls += static_cast<uint32>(NewPackets);
		for (int32 p = FirstPacket; p < Queue.Num(); ++p)
		{
			const FRenderPacket& Packet = Queue[p];
			Stats.ParticleTriangles += static_cast<uint64>(Packet.IndexCount / 3) * std::max(1u, Packet.InstanceCount);
		}
	}

	if (bDrewAny)
		++View.Stats->ParticleSystems;
}
