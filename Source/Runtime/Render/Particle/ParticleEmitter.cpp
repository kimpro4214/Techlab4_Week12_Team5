#include "EnginePCH.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleSpriteEmitterInstance.h"
#include "Render/Particle/ParticleModuleTypeDataBase.h"

FParticleEmitterInstance* UParticleEmitter::CreateInstance(UParticleSystemComponent* Component)
{
	// TypeData 모듈이 있으면 그 종류(Mesh 등)의 인스턴스를, 없으면 Sprite를 만든다
	UParticleLODLevel* LODLevel = GetLODLevel(0);
	UParticleModuleTypeDataBase* TypeData = LODLevel ? LODLevel->GetTypeDataModule() : nullptr;

	FParticleEmitterInstance* Instance = TypeData ? TypeData->CreateInstance() : nullptr;
	if (!Instance)
		Instance = new FParticleSpriteEmitterInstance();
	if (!Instance->Init(this, Component))
	{
		delete Instance;   // Required/Spawn 누락 등
		return nullptr;
	}
	return Instance;
}

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