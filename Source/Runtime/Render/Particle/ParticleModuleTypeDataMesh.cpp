#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleTypeDataMesh.h"
#include "Render/Particle/ParticleMeshEmitterInstance.h"

FParticleEmitterInstance* UParticleModuleTypeDataMesh::CreateInstance()
{
	return new FParticleMeshEmitterInstance(this);
}
