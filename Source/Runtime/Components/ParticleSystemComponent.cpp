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
	OnTransformDirty();
}

void UParticleSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	EmitterInstances.Reset();
	EmitterInstances.SetNum(ParticleSystem.Emitters.Num());
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	const FVector SpawnLocation = GetWorldLocation();
	bool bBoundsChanged = false;

	for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); ++EmitterIndex)
	{
		const FParticleEmitter& Emitter = ParticleSystem.Emitters[EmitterIndex];
		FParticleEmitterInstance& EmitterInstance = EmitterInstances[EmitterIndex];
		TArray<FParticle>& Particles = EmitterInstance.GetParticles();

		const int32 SpawnCount = EmitterInstance.ConsumeSpawnCount(Emitter, DeltaTime);
		bBoundsChanged |= !Particles.IsEmpty() || SpawnCount > 0;
		EmitterInstance.UpdateParticles(DeltaTime);

		for (int32 Index = 0; Index < SpawnCount; ++Index) 
		{
			FParticle Particle{};
			Particle.Location = SpawnLocation;
			Particle.Velocity = FVector(0.0f, 0.0f, 1.0f);
			Particle.Scale = 1.0f;
			Particle.Alpha = 1.0f;
			Particle.bAlive = true;

			Emitter.Lifetime.Spawn(Particle);
			Particles.Add(Particle);
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
		for (const FParticle& Particle : Instance.GetParticles())
		{
			const FVector Extent(Particle.Scale);
			WorldBounds.Expand(FBox{ Particle.Location - Extent, Particle.Location + Extent });
		}
	}

	return WorldBounds.GetWorldAABB(GetWorldMatrix().Inverse());
}

void UParticleSystemComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewContext& ViewContext)
{
	if (!QuadMesh || !Material) return;

	for(const FParticleEmitterInstance& Instance : EmitterInstances)
	{
		for (const FParticle& Particle : Instance.GetParticles())
		{
			const FMatrix Billboard = ViewContext.BuildBillboardMatrix(Particle.Location, Particle.Scale, Particle.Scale);

			UBillboardComponent::SubmitToRenderQueue(RenderQueue, Billboard);

			const FVector Delta = Particle.Location - ViewContext.CameraPosition;
			RenderQueue.Last().CameraToParticleDistance = Delta.Dot(Delta);
		}
	}
}
