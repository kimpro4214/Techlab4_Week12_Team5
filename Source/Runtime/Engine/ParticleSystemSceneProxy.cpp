#include "EnginePCH.h"
#include "Engine/ParticleSystemSceneProxy.h"

#include "Components/ParticleSystemComponent.h"
#include "Render/RenderPacket.h"

FParticleSystemSceneProxy::FParticleSystemSceneProxy(UParticleSystemComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent), ParticleComponent(InComponent)
{
}

void FParticleSystemSceneProxy::UpdateDynamicData()
{
	QuadMesh = ParticleComponent->QuadMesh;
	Material = ParticleComponent->GetMaterial(0);
	RenderParticles.Reset();

	for (const FParticleEmitterInstance& Instance : ParticleComponent->EmitterInstances)
	{
		for (int32 Index = 0; Index < Instance.GetActiveParticleCount(); ++Index)
		{
			const FBaseParticle Particle = Instance.GetParticle(Index);
			FSubUVConstants Params{ 0.0f, 1.0f, 1.0f, Particle.Alpha };
			Params.Tint = FVector4(Particle.Color, 1.0f);
			Params.DecodeSRGB = 0.0f;
			RenderParticles.Add({ Particle.Location, Particle.Scale, Params });
		}
	}
}

void FParticleSystemSceneProxy::GatherMeshElements(FRenderQueue& Queue, const FViewContext& View) const
{
	if (!QuadMesh || !Material) return;

	for (const FParticleRenderData& Particle : RenderParticles)
	{
		const FMatrix Billboard = View.BuildBillboardMatrix(Particle.Location, Particle.Scale, Particle.Scale);
		const FVector Delta = Particle.Location - View.CameraPosition;

		FRenderPacket& Packet = Queue.AddDefaulted_GetRef();
		Packet.Mesh = QuadMesh;
		Packet.Material = Material;
		Packet.Model = Queue.StoreWorldMatrix(Billboard);
		Packet.MaterialParamData = &Particle.Params;
		Packet.MaterialParamDataSize = sizeof(FSubUVConstants);
		Packet.CameraToParticleDistance = Delta.Dot(Delta);
	}
}
