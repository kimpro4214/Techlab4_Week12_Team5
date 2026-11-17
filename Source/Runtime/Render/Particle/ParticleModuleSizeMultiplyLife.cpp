#include "EnginePCH.h"
#include "Curves/CurveFloat.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Render/Particle/ParticleModuleSizeMultiplyLife.h"
#include "ParticleModuleSizeMultiplyLife.h"


void UParticleModuleSizeMultiplyLife::Spawn(const FSpawnContext& Context)
{
	FBaseParticle& Particle = *Context.ParticleBase;
	const float Age = Particle.RelativeTime + Context.SpawnTime * Particle.OneOverMaxLifetime;
	Particle.Size *= GetSizeScale(Age);
}

void UParticleModuleSizeMultiplyLife::Update(const FUpdateContext& Context)
{
	FParticleEmitterInstance* Owner = &Context.Owner;
	BEGIN_UPDATE_LOOP
		Particle.Size *= GetSizeScale(Particle.RelativeTime);
	END_UPDATE_LOOP
}

float UParticleModuleSizeMultiplyLife::GetSizeScale(float Age) const
{
	if (!SizeScaleCurve)
	{
		return 1.0f;
	}

	return std::max(0.0f, SizeScaleCurve->GetFloatValue(std::clamp(Age, 0.0f, 1.0f)));
}
