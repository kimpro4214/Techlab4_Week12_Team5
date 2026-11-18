// Render/Particle/Modules/Location/ParticleModuleLocation.cpp
#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleLocation.h"
#include "Render/Particle/ParticleEmitterInstance.h"

void UParticleModuleLocation::Spawn(const FSpawnContext& Context)
{
	// PreSpawn이 이미 이미터 원점을 넣어뒀으니 오프셋만 더한다.
	// 오프셋은 이미터 로컬 기준이라, 컴포넌트 회전을 따라가도록 시뮬레이션 공간으로 돌린다
	const FVector LocalOffset = ParticleRandRange(StartLocationMin, StartLocationMax);
	Context.ParticleBase->Location += Context.Owner.EmitterToSimulation.TransformVector(LocalOffset);
}
