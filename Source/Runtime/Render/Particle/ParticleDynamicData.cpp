#include "EnginePCH.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Render/RenderCommand.h"

void FDynamicSpriteEmitterData::Init(int32 MaxParticles)
{
	// 이번 프레임의 뷰 버퍼를 처음부터 다시 쓴다
	NextViewIndex = 0;

	if (MaxParticles <= Capacity)
		return;

	// 용량이 늘었으면 버퍼를 새 크기로 다시 만든다. VB는 AcquireViewResources에서 필요할 때 만든다
	Capacity = MaxParticles;
	ViewResources.Reset();
	BuildIndexBuffer(Capacity);
}

void FDynamicSpriteEmitterData::BuildIndexBuffer(int32 MaxParticles)
{
	// 파티클당 정점 4개 → 최대 65535 × 4 = 262140개라 uint16을 넘는다. 반드시 uint32
	TArray<uint32> Indices;
	Indices.SetNum(MaxParticles * 6);
	for (int32 i = 0; i < MaxParticles; ++i)
	{
		const uint32 Base = static_cast<uint32>(i) * 4;
		Indices[i * 6 + 0] = Base + 0;
		Indices[i * 6 + 1] = Base + 1;
		Indices[i * 6 + 2] = Base + 2;
		Indices[i * 6 + 3] = Base + 0;
		Indices[i * 6 + 4] = Base + 2;
		Indices[i * 6 + 5] = Base + 3;
	}
	IndexBuffer = RenderCommand::CreateStaticIndexBuffer(Indices.GetData(), static_cast<uint32>(MaxParticles * 6));
}

FDynamicSpriteEmitterData::FViewResources* FDynamicSpriteEmitterData::AcquireViewResources()
{
	if (NextViewIndex >= MaxViewsPerFrame)
		return nullptr;

	// 처음 보는 뷰 번호면 그 뷰 전용 VB를 만든다. 이후 프레임부터는 재사용
	if (NextViewIndex >= ViewResources.Num())
	{
		TUniquePtr<FViewResources> NewResources = MakeUnique<FViewResources>();
		NewResources->VertexBuffer = RenderCommand::CreateDynamicVertexBuffer(
			static_cast<uint32>(Capacity * 4 * sizeof(FParticleSpriteVertex)),
			static_cast<uint32>(sizeof(FParticleSpriteVertex)));
		ViewResources.Add(std::move(NewResources));
	}

	FViewResources* Resources = ViewResources[NextViewIndex++].get();
	return Resources->VertexBuffer ? Resources : nullptr;
}

void FDynamicSpriteEmitterData::Submit(FRenderQueue& RenderQueue, const FViewContext& View)
{
	// C7에서 구현: 정점 채우기 → VB 업로드 → 렌더 패킷 추가
}
