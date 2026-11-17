#include "EnginePCH.h"
#include "Core/HAL/UnrealMemory.h"
#include "Render/Particle/ParticleData.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleSpriteEmitterInstance.h"
#include "Render/Particle/ParticleModule.h"

void UParticleEmitter::AddLODLevel(UParticleLODLevel* InLODLevel)
{
	if (!InLODLevel || LODLevels.Find(InLODLevel) >= 0)
	{
		return;
	}

	if (InLODLevel->GetOuter() && InLODLevel->GetOuter() != this)
	{
		return;
	}

	InLODLevel->SetOuter(this);
	LODLevels.Add(InLODLevel);
}

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
	RequiredInstanceBytes = 0;
	ModuleInstanceOffsetMap.Reset();
	ModuleOffsetMap.Reset();
	ParticleSize = sizeof(FBaseParticle);

	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (LODLevel) LODLevel->UpdateModuleLists();
	}

	const UParticleLODLevel* HighestLOD = GetLODLevel(0);
	if (!HighestLOD)
		return;

	for (const UParticleModule* Module : HighestLOD->GetModules())
	{
		if (!Module)
			continue;

		const uint32 InstanceBytes = Module->RequiredBytesPerInstance();
		if (InstanceBytes > 0)
		{
			const uint64 Offset = FMemory::QuantizeSize(RequiredInstanceBytes, Module->GetInstancePayloadAlignment());
			ModuleInstanceOffsetMap.Add(Module, Offset);
			RequiredInstanceBytes = Offset + InstanceBytes;
		}

		const uint32 Bytes = Module->RequiredBytes(HighestLOD->GetTypeDataModule());
		if (Bytes == 0)
			continue;

		const int32 Offset = static_cast<int32>(FMemory::QuantizeSize(ParticleSize, Module->GetParticlePayloadAlignment()));

		ModuleOffsetMap.Add(Module, Offset);
		ParticleSize = Offset + static_cast<int32>(Bytes);
	}
}
