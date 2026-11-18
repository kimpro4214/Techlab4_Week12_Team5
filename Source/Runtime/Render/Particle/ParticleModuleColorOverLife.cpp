// Render/Particle/Modules/Color/ParticleModuleColorOverLife.cpp
#include "EnginePCH.h"
#include "Curves/CurveFloat.h"
#include "Render/Particle/ParticleModuleColorOverLife.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include <algorithm>

void UParticleModuleColorOverLife::Update(const FUpdateContext& Context)
{
	FParticleEmitterInstance* Owner = &Context.Owner;
	BEGIN_UPDATE_LOOP
		const float Age = std::clamp(Particle.RelativeTime, 0.0f, 1.0f);
		const float T = ColorBlendCurve
			? std::clamp(ColorBlendCurve->GetFloatValue(Age), 0.0f, 1.0f)
			: Age;
	Particle.Color.R = StartColor.R + (EndColor.R - StartColor.R) * T;
	Particle.Color.G = StartColor.G + (EndColor.G - StartColor.G) * T;
	Particle.Color.B = StartColor.B + (EndColor.B - StartColor.B) * T;
	Particle.Color.A = StartColor.A + (EndColor.A - StartColor.A) * T;
	END_UPDATE_LOOP
}
