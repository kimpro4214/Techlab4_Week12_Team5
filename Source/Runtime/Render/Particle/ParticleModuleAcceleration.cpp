#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleAcceleration.h"
#include "Render/Particle/ParticleEmitterInstance.h"

void UParticleModuleAcceleration::Update(const FUpdateContext& Context)
{
	FParticleEmitterInstance* Owner = &Context.Owner;
	const FVector VelocityDelta = Acceleration * Context.DeltaTime;

	BEGIN_UPDATE_LOOP
		// 다음 프레임은 BaseVelocity에서 시작하므로 누적 속도도 함께 갱신합니다.
		Particle.BaseVelocity += VelocityDelta;
		Particle.Velocity += VelocityDelta;
	END_UPDATE_LOOP
}
