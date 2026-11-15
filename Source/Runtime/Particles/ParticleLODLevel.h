#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleLODLevel.generated.h"

class UParticleModule;
class UParticleModuleRequired;
class UParticleModuleSpawn;
class UParticleModuleTypeDataBase;

UCLASS()
class UParticleLODLevel : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UParticleLODLevel() override;

	int32 GetLevel() const { return Level; }

	bool IsEnabled() const { return bEnabled; }
	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

	// 새 LOD에 필요한 기본 모듈(Required, Spawn)을 생성합니다.
	void InitializeDefaultModules();

	// 모듈을 생성합니다.
	UParticleModule* AddModule(UClass* ModuleClass);

	// 모듈을 제거합니다. Required와 Spawn은 제거할 수 없습니다.
	bool RemoveModule(UParticleModule* Module);

	bool Validate() const;
	bool BuildModuleLists();

	UParticleModuleRequired* GetRequiredModule() const
	{
		return RequiredModule;
	}

	UParticleModuleSpawn* GetSpawnModule() const
	{
		return SpawnModule;
	}

	UParticleModuleTypeDataBase* GetTypeDataModule() const
	{
		return TypeDataModule;
	}

	const TArray<UParticleModule*>& GetModules() const
	{
		return Modules;
	}

	const TArray<UParticleModule*>& GetSpawnModules() const
	{
		return SpawnModules;
	}

	const TArray<UParticleModule*>& GetUpdateModules() const
	{
		return UpdateModules;
	}

private:
	// LOD Level은 Emitter가 배열 순서에 맞춰 직접 지정함.
	friend class UParticleEmitter;

	void ClearModuleLists();

	int32 Level = 0;
	bool bEnabled = true;
	
	// 특수 모듈을 포함한 모든 모듈의 유일한 소유 배열입니다.
	TArray<UParticleModule*> Modules;

	// Modules 순서를 보존하여 구성하는 비소유 실행 목록입니다.
	TArray<UParticleModule*> SpawnModules;
	TArray<UParticleModule*> UpdateModules;

	// 소유 배열 Modules의 객체를 가리키는 비소유 포인터입니다.
	UParticleModuleRequired* RequiredModule = nullptr;
	UParticleModuleSpawn* SpawnModule = nullptr;
	UParticleModuleTypeDataBase* TypeDataModule = nullptr;
};