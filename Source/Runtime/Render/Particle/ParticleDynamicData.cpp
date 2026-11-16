#include "EnginePCH.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Render/RenderCommand.h"
#include "Render/Material.h"
#include <algorithm>

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
	const int32 Count = std::min(Source.ActiveParticleCount, Capacity);
	if (Count <= 0 || !Source.Material || !IndexBuffer)
		return;

	// 뷰마다 별도 VB. 여러 뷰를 제출한 뒤 한꺼번에 그리므로 하나를 덮어쓰면 앞 뷰가 깨진다
	FViewResources* Resources = AcquireViewResources();
	if (!Resources)
		return;

	// 1) 파티클 하나당 정점 4개로 펼친다. 실제 코너 위치 계산은 VS가 카메라 축으로 한다
	//    순서는 인덱스 패턴(0,1,2 / 0,2,3)과 맞물린다: 좌하 → 좌상 → 우상 → 우하
	static const FVector2 Corners[4] = {
		FVector2(-0.5f, -0.5f), FVector2(-0.5f, 0.5f), FVector2(0.5f, 0.5f), FVector2(0.5f, -0.5f) };

	ScratchVertices.SetNum(Count * 4, false);
	for (int32 i = 0; i < Count; ++i)
	{
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			Source.ParticleData + Source.ParticleIndices[i] * Source.ParticleStride);

		for (int32 c = 0; c < 4; ++c)
		{
			FParticleSpriteVertex& Vertex = ScratchVertices[i * 4 + c];
			Vertex.Position     = Particle.Location;
			Vertex.RelativeTime = Particle.RelativeTime;
			Vertex.Size         = FVector2(Particle.Size.X, Particle.Size.Y);
			Vertex.Corner       = Corners[c];
			Vertex.Rotation     = Particle.Rotation;
			Vertex.Color        = FVector4(Particle.Color.R, Particle.Color.G, Particle.Color.B, Particle.Color.A);
		}
	}

	// 2) 이 뷰 전용 VB에 올린다 (Map WRITE_DISCARD)
	RenderCommand::UpdateBufferData(Resources->VertexBuffer.get(), ScratchVertices.GetData(),
		static_cast<uint32>(Count * 4 * sizeof(FParticleSpriteVertex)));

	// 3) 카메라 축과 블렌드 방식 상수 (b1)
	View.GetBillboardAxes(Resources->Constants.CameraRight, Resources->Constants.CameraUp);
	Resources->Constants.SoftCircle       = 0.0f;
	Resources->Constants.PremultiplyAlpha = Source.Material->BlendState == EBlendState::Additive ? 1.0f : 0.0f;

	// 4) 이미터 하나 = 패킷 하나 = DrawIndexed 한 번
	const FVector Delta = Source.EmitterWorldOrigin - View.CameraPosition;

	FRenderPacket Packet;
	Packet.Model                    = RenderQueue.StoreWorldMatrix(Source.SimulationToWorld);   // b2 World
	Packet.Geometry.VertexBuffer    = Resources->VertexBuffer.get();
	Packet.Geometry.IndexBuffer     = IndexBuffer.get();
	Packet.IndexCount               = static_cast<uint32>(Count * 6);
	Packet.Material                 = Source.Material;
	Packet.MaterialParamData        = &Resources->Constants;
	Packet.MaterialParamDataSize    = sizeof(FParticleSpriteConstants);
	Packet.CameraToParticleDistance = Delta.Dot(Delta);   // 이미터끼리의 뒤→앞 순서
	RenderQueue.Add(Packet);
}
