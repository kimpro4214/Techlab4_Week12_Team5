#include "EnginePCH.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModule.h"

void UParticleLODLevel::UpdateModuleLists()
{
	SpawnModules.Reset();	
	UpdateModules.Reset();

	for (UParticleModule* Module : Modules)
	{
		if (!Module || !Module->IsEnabled())
			continue;
		// 한 모듈이 둘 다일 수 있다 (예: Color는 Spawn에서 초기색, Update에서 변화)
		if (Module->IsSpawnModule())
			SpawnModules.Add(Module);
		if (Module->IsUpdateModule())
			UpdateModules.Add(Module);
		// 추가
	}
}	