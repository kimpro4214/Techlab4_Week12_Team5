#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleLODLevel.generated.h"

class UParticleModuleSpawn;
class UParticleModuleRequired;
class UParticleModuleTypeDataBase; // 아직 정의하지 않음. Mesh / Beam / Ribbon TypeData의 기반 클래스 자리

UCLASS()
class UParticleLODLevel : public UObject
{
	GENERATED_BODY()

public:
	const TArray<UParticleModule*>& GetModules() const { return Modules; }
	UParticleModuleRequired* GetRequiredModule() const { return RequiredModule; }
	UParticleModuleSpawn* GetSpawnModule() const { return SpawnModule; }
	UParticleModuleTypeDataBase* GetTypeDataModule() const { return TypeDataModule; }

	void SetRequiredModule(UParticleModuleRequired* InModule) { RequiredModule = InModule; }
	void SetSpawnModule(UParticleModuleSpawn* InModule) { SpawnModule = InModule; }
	void AddModule(UParticleModule* InModule) { if (InModule) Modules.Add(InModule); }

private:
	int32 Level = 0;	// 현재 LOD Level
	bool bEnabled = true;
	UParticleModuleRequired* RequiredModule = nullptr;
	UParticleModuleSpawn* SpawnModule = nullptr;
	UParticleModuleTypeDataBase* TypeDataModule = nullptr;
	TArray<UParticleModule*> Modules;
};
