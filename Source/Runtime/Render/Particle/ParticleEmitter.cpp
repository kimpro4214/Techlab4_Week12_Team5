#include "EnginePCH.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleSpriteEmitterInstance.h"

FParticleEmitterInstance* UParticleEmitter::CreateInstance(UParticleSystemComponent* Component)
{
	// 지금은 Sprite만 있다. Mesh 이미터가 생기면 TypeData 모듈이 인스턴스 종류를 고른다
	FParticleEmitterInstance* Instance = new FParticleSpriteEmitterInstance();
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