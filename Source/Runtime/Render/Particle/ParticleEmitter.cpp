#include "EnginePCH.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleLODLevel.h"

UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 Index) const
{
	if (Index < 0 || Index >= LODLevels.Num())
		return nullptr;
	return LODLevels[Index];
}

void UParticleEmitter::CacheEmitterModuleInfo()
{
	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (LODLevel)
			LODLevel->UpdateModuleLists();
	}
}