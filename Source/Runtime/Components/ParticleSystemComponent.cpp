#include "EnginePCH.h"
#include "Render/RenderPacket.h"
#include "Components/ParticleSystemComponent.h"

UParticleSystemComponent::UParticleSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bCastShadow = false;
	bReceivesDecals = false;
}

void UParticleSystemComponent::SetParticleSystem(const FParticleSystem& InSystem)
{
	ParticleSystem = InSystem;
	EmitterInstances.Reset();
	EmitterInstances.SetNum(ParticleSystem.Emitters.Num());
	for (int32 Index = 0; Index < EmitterInstances.Num(); ++Index) 
	{
		EmitterInstances[Index].Init(ParticleSystem.Emitters[Index]);
	}
	ParticleRenderParams.Reset();
	OnTransformDirty();
}

void UParticleSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	EmitterInstances.Reset();
	EmitterInstances.SetNum(ParticleSystem.Emitters.Num());
	for (int32 Index = 0; Index < EmitterInstances.Num(); ++Index)
	{
		EmitterInstances[Index].Init(ParticleSystem.Emitters[Index]);
	}
	ParticleRenderParams.Reset();
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	const FVector SpawnLocation = GetWorldLocation();
	bool bBoundsChanged = false;
	ParticleRenderParams.Reset();

	for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); ++EmitterIndex)
	{
		const FParticleEmitter& Emitter = ParticleSystem.Emitters[EmitterIndex];
		FParticleEmitterInstance& EmitterInstance = EmitterInstances[EmitterIndex];

		const int32 SpawnCount = EmitterInstance.ConsumeSpawnCount(Emitter, DeltaTime);
		bBoundsChanged |= EmitterInstance.GetActiveParticleCount() > 0 || SpawnCount > 0;
		EmitterInstance.UpdateParticles(DeltaTime);

		for (int32 Index = 0; Index < SpawnCount; ++Index)
		{
			FBaseParticle Particle{};
			Particle.bAlive = true;

			Emitter.Location.Spawn(Particle, SpawnLocation);
			Emitter.Velocity.Spawn(Particle);
			Emitter.Size.Spawn(Particle);
			Emitter.Color.Spawn(Particle);
			Emitter.Lifetime.Spawn(Particle);
			EmitterInstance.AddParticle(Particle);
		}

		// 렌더 패킷이 참조할 상수를 제출 순서대로 준비한다.
		for (int32 Index = 0; Index < EmitterInstance.GetActiveParticleCount(); ++Index)
		{
			const FBaseParticle Particle = EmitterInstance.GetParticle(Index);
			FSubUVConstants Params{ 0.0f, 1.0f, 1.0f, Particle.Alpha };
			Params.Tint = FVector4(Particle.Color, 1.0f);
			Params.DecodeSRGB = 0.0f;
			ParticleRenderParams.Add(Params);
		}
	}

	if (bBoundsChanged)
	{
		OnTransformDirty();
	}
}

FBox UParticleSystemComponent::CalcLocalBounds() const
{
	const FVector Origin = GetWorldLocation();
	FBox WorldBounds{ Origin, Origin };
	
	for(const FParticleEmitterInstance& Instance : EmitterInstances)
	{
		for (int32 Index = 0; Index < Instance.GetActiveParticleCount(); ++Index)
		{
			const FBaseParticle Particle = Instance.GetParticle(Index);
			const FVector Extent(Particle.Scale);
			WorldBounds.Expand(FBox{ Particle.Location - Extent, Particle.Location + Extent });
		}
	}

	return WorldBounds.GetWorldAABB(GetWorldMatrix().Inverse());
}

void UParticleSystemComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext)
{
	if (!QuadMesh || !Material) return;

	int32 ParamIndex = 0;
	for(const FParticleEmitterInstance& Instance : EmitterInstances)
	{
		for (int32 Index = 0; Index < Instance.GetActiveParticleCount(); ++Index)
		{
			const FBaseParticle Particle = Instance.GetParticle(Index);
			const FMatrix Billboard = ViewContext.BuildBillboardMatrix(Particle.Location, Particle.Scale, Particle.Scale);

			UBillboardComponent::SubmitToRenderQueue(RenderQueue, Billboard);

			FRenderPacket& Packet = RenderQueue.Last();
			Packet.MaterialParamData = &ParticleRenderParams[ParamIndex++];
			const FVector Delta = Particle.Location - ViewContext.CameraPosition;
			Packet.CameraToParticleDistance = Delta.Dot(Delta);
		}
	}
}
