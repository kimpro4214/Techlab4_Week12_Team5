#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleCollision.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Components/ParticleSystemComponent.h"
#include "Collision/HitResult.h"
#include "Collision/Ray.h"
#include "Engine/World.h"


void UParticleModuleCollision::Update(const FUpdateContext& Context)
{
	FParticleEmitterInstance* Owner = &Context.Owner;
	UParticleSystemComponent* Component = Owner->Component;
	UWorld* World = Component ? Component->GetWorld() : nullptr;
	if (!World || Context.DeltaTime <= 0.0f) return;

	const FMatrix WorldToSimulation = Owner->SimulationToWorld.Inverse();

	BEGIN_UPDATE_LOOP
		const FVector Start = Owner->SimulationToWorld.TransformPosition(Particle.Location);
		const FVector End = Owner->SimulationToWorld.TransformPosition(Particle.Location + Particle.Velocity * Context.DeltaTime);
		const FVector Delta = End - Start;
		const float Distance = Delta.Length();
		if (Distance <= SMALL_NUMBER) continue;

		const FRay Ray{ Start, Delta / Distance};
		FHitResult Hit;
		if (!World->SweepSingle(Ray, Distance, 0.0f, Hit, Component->GetOwner(), true)) continue;

		if (Hit.bStartPentrating)
		{
			Owner->KillParticle(i);
			continue;
		}

		constexpr float Damping = 0.6f;
		constexpr float SurfaceOffset = 0.001f;

		const auto Reflect = [&Hit](const FVector& Velocity)
		{
				return Velocity - Hit.Normal * (2.0f * Velocity.Dot(Hit.Normal));
		};

		const FVector WorldVelocity = Delta / Context.DeltaTime;
		const FVector ReflectedVelocity = Reflect(WorldVelocity) * Damping;
		const float RemainingTime = Context.DeltaTime * (1.0f - Hit.Time);

		const FVector NewWorldPosition = Hit.Location + Hit.Normal * SurfaceOffset + ReflectedVelocity * RemainingTime;
		Particle.Location = WorldToSimulation.TransformPosition(NewWorldPosition);

		const FVector WorldBaseVelocity = Owner->SimulationToWorld.TransformVector(Particle.BaseVelocity);
		Particle.BaseVelocity = WorldToSimulation.TransformVector(Reflect(WorldBaseVelocity) * Damping);

		Particle.Velocity = FVector::ZeroVector;
	END_UPDATE_LOOP
}
