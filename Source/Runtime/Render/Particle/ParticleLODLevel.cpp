#include "EnginePCH.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModule.h"

void UParticleLODLevel::SetRequiredModule(UParticleModuleRequired* InModule)
{
	if (InModule && InModule->GetOuter() && InModule->GetOuter() != this)
	{
		return;
	}

	RequiredModule = InModule;
	if (InModule)
	{
		InModule->SetOuter(this);
	}
}

void UParticleLODLevel::SetSpawnModule(UParticleModuleSpawn* InModule)
{
	if (InModule && InModule->GetOuter() && InModule->GetOuter() != this)
	{
		return;
	}

	SpawnModule = InModule;
	if (InModule)
	{
		InModule->SetOuter(this);
	}
}

void UParticleLODLevel::AddModule(UParticleModule* InModule)
{
	if (!InModule || Modules.Find(InModule) >= 0)
	{
		return;
	}

	if (InModule->GetOuter() && InModule->GetOuter() != this)
	{
		return;
	}

	InModule->SetOuter(this);
	Modules.Add(InModule);
}

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
	}
}
