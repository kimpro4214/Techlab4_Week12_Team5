#include "EnginePCH.h"
#include "Components/ParticleSystemComponent.h"
#include "Engine/ParticleSystemSceneProxy.h"

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

FPrimitiveSceneProxy* UParticleSystemComponent::CreateSceneProxy()
{
	return new FParticleSystemSceneProxy(this);
}
