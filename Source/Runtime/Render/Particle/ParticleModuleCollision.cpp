#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleCollision.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Components/ParticleSystemComponent.h"
#include "Collision/HitResult.h"
#include "Collision/Ray.h"
#include "Engine/World.h"

namespace
{
	// 충돌 시작 위치가 블로킹 박스 안이면 호출자가 입자를 제거한다.
	bool ResolveParticleCollision(FParticleEmitterInstance& Owner, FBaseParticle& Particle, float MoveTime)
	{
		const UParticleSystemComponent* Component = Owner.Component;
		UWorld* World = Component ? Component->GetWorld() : nullptr;
		if (!World || MoveTime <= 0.0f)
			return false;

		const FMatrix& SimulationToWorld = Owner.SimulationToWorld;
		const FVector Start = SimulationToWorld.TransformPosition(Particle.Location);
		const FVector End = SimulationToWorld.TransformPosition(Particle.Location + Particle.Velocity * MoveTime);
		const FVector Delta = End - Start;
		const float Distance = Delta.Length();
		if (Distance <= SMALL_NUMBER)
			return false;

		const FRay Ray{ Start, Delta / Distance };
		FHitResult Hit;
		if (!World->SweepSingle(Ray, Distance, 0.0f, Hit, Component->GetOwner(), true))
			return false;

		const FVector WorldVelocity = Delta / MoveTime;

		FParticleCollisionEvent Event;
		Event.ParticleId = Particle.ParticleId;
		Event.HitComponentId = Hit.HitComponent->GetUUID();
		Event.WorldPosition = Hit.ImpactPoint;
		Event.WorldNormal = Hit.ImpactNormal;
		Event.IncomingWorldVelocity = WorldVelocity;

		Owner.AddCollisionEvent(Event);

		if (Hit.bStartPentrating)
			return true;

		constexpr float Damping = 0.6f;
		constexpr float SurfaceOffset = 0.001f;

		const auto Reflect = [&Hit](const FVector& Velocity)
		{
			return Velocity - Hit.Normal * (2.0f * Velocity.Dot(Hit.Normal));
		};

		const FVector ReflectedVelocity = Reflect(WorldVelocity) * Damping;
		const float RemainingTime = MoveTime * (1.0f - Hit.Time);

		const FVector NewWorldPosition = Hit.Location + Hit.Normal * SurfaceOffset + ReflectedVelocity * RemainingTime;
		const FMatrix WorldToSimulation = SimulationToWorld.Inverse();
		Particle.Location = WorldToSimulation.TransformPosition(NewWorldPosition);

		const FVector WorldBaseVelocity = SimulationToWorld.TransformVector(Particle.BaseVelocity);
		Particle.BaseVelocity = WorldToSimulation.TransformVector(Reflect(WorldBaseVelocity) * Damping);

		Particle.Velocity = FVector::ZeroVector;
		return false;
	}
}

void UParticleModuleCollision::Spawn(const FSpawnContext& Context)
{
	if (ResolveParticleCollision(Context.Owner, *Context.ParticleBase, Context.SpawnTime))
		Context.ParticleBase->RelativeTime = 1.0f;
}

void UParticleModuleCollision::Update(const FUpdateContext& Context)
{
	FParticleEmitterInstance* Owner = &Context.Owner;
	BEGIN_UPDATE_LOOP
		if (ResolveParticleCollision(*Owner, Particle, Context.DeltaTime))
			Owner->KillParticle(i);
	END_UPDATE_LOOP
}
