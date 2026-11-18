#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleTypeDataRibbon.h"
#include "Render/Particle/ParticleRibbonEmitterInstance.h"

FParticleEmitterInstance* UParticleModuleTypeDataRibbon::CreateInstance()
{
	return new FParticleRibbonEmitterInstance(this);
}
