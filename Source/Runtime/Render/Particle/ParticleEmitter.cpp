#include "EnginePCH.h"
#include "Core/HAL/UnrealMemory.h"
#include "Render/Particle/ParticleData.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Components/ParticleSystemComponent.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleSpriteEmitterInstance.h"
#include "Render/Particle/ParticleModuleTypeDataBase.h"
#include "Render/Particle/ParticleModule.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Serialization/Archive.h"

void UParticleEmitter::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsError() || !Ar.HasAnyPortFlags(EPropertyPortFlags::PPF_Asset)) return;

	// 기존 애셋의 이미터 레코드에는 이름이 없으므로 남은 데이터가 있을 때만 읽습니다.
	if (Ar.IsLoading())
	{
		EmitterName.clear();
		if (Ar.Tell() < Ar.TotalSize()) Ar << EmitterName;
	}
	else if (!EmitterName.empty())
	{
		Ar << EmitterName;
	}
}

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
	if (!Component)
		return nullptr;

	// TypeData 모듈이 있으면 그 종류(Mesh 등)의 인스턴스를, 없으면 Sprite를 만든다
	UParticleLODLevel* LODLevel = GetLODLevel(Component->GetEmitterLODIndex());
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

void UParticleEmitter::CacheEmitterModuleInfo(int32 LODIndex)
{
	RequiredInstanceBytes = 0;
	ModuleInstanceOffsetMap.Reset();
	ModuleOffsetMap.Reset();
	ParticleSize = sizeof(FBaseParticle);

	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (LODLevel) LODLevel->UpdateModuleLists();
	}

	const UParticleLODLevel* ActiveLOD = GetLODLevel(LODIndex);
	if (!ActiveLOD)
		return;

	const TArray<UParticleModule*>& ActiveModules = ActiveLOD->GetModules();
	for (const UParticleModule* Module : ActiveModules)
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

		const uint32 Bytes = Module->RequiredBytes(ActiveLOD->GetTypeDataModule());
		if (Bytes == 0)
			continue;

		const int32 Offset = static_cast<int32>(FMemory::QuantizeSize(ParticleSize, Module->GetParticlePayloadAlignment()));

		ModuleOffsetMap.Add(Module, Offset);
		ParticleSize = Offset + static_cast<int32>(Bytes);
	}

	// 호환성 검사는 LOD 0 기준이므로 LOD 0의 오프셋만 다른 LOD에 공유한다.
	if (LODIndex != 0)
		return;

	for (int32 TargetLODIndex = 1; TargetLODIndex < LODLevels.Num(); ++TargetLODIndex)
	{
		if (!IsLODCompatible(TargetLODIndex))
			continue;

		const TArray<UParticleModule*>& TargetModules = LODLevels[TargetLODIndex]->GetModules();
		for (int32 Slot = 0; Slot < ActiveModules.Num(); ++Slot)
		{
			const UParticleModule* BaseModule = ActiveModules[Slot];
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
