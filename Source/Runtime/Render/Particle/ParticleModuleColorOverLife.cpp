// Render/Particle/Modules/Color/ParticleModuleColorOverLife.cpp
#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleColorOverLife.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include <algorithm>

void UParticleModuleColorOverLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	BEGIN_UPDATE_LOOP
		const float T = std::min(Particle.RelativeTime, 1.0f);
	Particle.Color.R = StartColor.R + (EndColor.R - StartColor.R) * T;
	Particle.Color.G = StartColor.G + (EndColor.G - StartColor.G) * T;
	Particle.Color.B = StartColor.B + (EndColor.B - StartColor.B) * T;
	Particle.Color.A = StartColor.A + (EndColor.A - StartColor.A) * T;
	END_UPDATE_LOOP
}