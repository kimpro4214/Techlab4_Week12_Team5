// Render/Particle/Modules/Velocity/ParticleModuleVelocity.cpp
#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleVelocity.h"
#include "Render/Particle/ParticleEmitterInstance.h"

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	FVector Velocity = ParticleRandRange(StartVelocityMin, StartVelocityMax);
	if (!bInWorldSpace)
		Velocity = Owner->EmitterToSimulation.TransformVector(Velocity);

	// 덮어쓰지 않고 더한다 → 속도 모듈 여러 개를 쌓을 수 있다 
	ParticleBase->BaseVelocity += Velocity;
	ParticleBase->Velocity += Velocity;
}