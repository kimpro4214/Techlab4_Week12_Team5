#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleTypeDataBeam.h"
#include "Render/Particle/ParticleBeamEmitterInstance.h"

FParticleEmitterInstance* UParticleModuleTypeDataBeam::CreateInstance()
{
	return new FParticleBeamEmitterInstance(this);
}
