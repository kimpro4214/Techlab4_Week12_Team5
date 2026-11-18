#include "EnginePCH.h"
#include "Core/HAL/UnrealMemory.h"
#include "Render/Particle/ParticleData.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleSpriteEmitterInstance.h"
#include "Render/Particle/ParticleModuleTypeDataBase.h"
#include "Render/Particle/ParticleModule.h"
#include "Render/Particle/ParticleModuleRequired.h"

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

	const TArray<UParticleModule*>& BaseModules = HighestLOD->GetModules();
	for (const UParticleModule* Module : BaseModules)
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

	for (int32 LODIndex = 1; LODIndex < LODLevels.Num(); ++LODIndex)
	{
		if (!IsLODCompatible(LODIndex))
			continue;

		const TArray<UParticleModule*>& TargetModules = LODLevels[LODIndex]->GetModules();
		for (int32 Slot = 0; Slot < BaseModules.Num(); ++Slot)
		{
			const UParticleModule* BaseModule = BaseModules[Slot];
			const UParticleModule* TargetModule = TargetModules[Slot];
			if (const int32* ParticleOffset = ModuleOffsetMap.Find(BaseModule))
				ModuleOffsetMap.Add(TargetModule, *ParticleOffset);
			if (const uint64* InstanceOffset = ModuleInstanceOffsetMap.Find(BaseModule))
				ModuleInstanceOffsetMap.Add(TargetModule, *InstanceOffset);
		}
	}
}

bool UParticleEmitter::IsLODCompatible(int32 Index) const
{
	const UParticleLODLevel* BaseLOD = GetLODLevel(0);
	const UParticleLODLevel* TargetLOD = GetLODLevel(Index);
	if (!BaseLOD || !TargetLOD)
		return false;

	const UParticleModuleRequired* BaseRequired = BaseLOD->GetRequiredModule();
	const UParticleModuleRequired* TargetRequired = TargetLOD->GetRequiredModule();
	if (!BaseRequired || !TargetRequired || !TargetLOD->GetSpawnModule())
		return false;

	const UParticleModuleTypeDataBase* TypeData = BaseLOD->GetTypeDataModule();
	if (TypeData != TargetLOD->GetTypeDataModule()
		|| BaseRequired->IsUseLocalSpace() != TargetRequired->IsUseLocalSpace()
		|| BaseRequired->GetMaxParticles() != TargetRequired->GetMaxParticles()
		|| BaseRequired->GetEmitterLoops() != TargetRequired->GetEmitterLoops())
		return false;

	const TArray<UParticleModule*>& BaseModules = BaseLOD->GetModules();
	const TArray<UParticleModule*>& TargetModules = TargetLOD->GetModules();
	if (BaseModules.Num() != TargetModules.Num())
		return false;

	for (int32 Slot = 0; Slot < BaseModules.Num(); ++Slot)
	{
		const UParticleModule* BaseModule = BaseModules[Slot];
		const UParticleModule* TargetModule = TargetModules[Slot];
		if (!BaseModule || !TargetModule)
			return false;

		const UClass* BaseModuleClass = BaseModule->GetClass();
		if (!BaseModuleClass || BaseModuleClass != TargetModule->GetClass())
			return false;

		const bool bSameParticleLayout = BaseModule->RequiredBytes(TypeData) == TargetModule->RequiredBytes(TypeData) && BaseModule->GetParticlePayloadAlignment() == TargetModule->GetParticlePayloadAlignment();

		const bool bSameInstanceLayout = BaseModule->RequiredBytesPerInstance() == TargetModule->RequiredBytesPerInstance() && BaseModule->GetInstancePayloadAlignment() == TargetModule->GetInstancePayloadAlignment();

		if (!bSameParticleLayout || !bSameInstanceLayout)
			return false;
	}

	return true;
}
