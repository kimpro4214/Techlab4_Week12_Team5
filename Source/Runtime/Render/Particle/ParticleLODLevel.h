#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleSpawn.h"
#include "ParticleLODLevel.generated.h"

class UParticleModule;
class UParticleModuleRequired;
class UParticleModuleSpawn;
class UParticleModuleTypeDataBase;


// Emitter의 LOD 하나에 해당하는 모듈 묶음입니다.
// 하나의 Emitter가 여러 개의 LODLevel을 갖는다. LODLevel은 SpawnModules / UpdateModules를 캐시한다.
UCLASS()
class UParticleLODLevel : public UObject
{
	GENERATED_BODY()
public:
	bool IsEnabled() const { return bEnabled; }
	void SetEnabled(bool bValue) { bEnabled = bValue; }

	UParticleModuleRequired* GetRequiredModule() const { return RequiredModule; }
	UParticleModuleSpawn* GetSpawnModule() const { return SpawnModule; }
	UParticleModuleTypeDataBase* GetTypeDataModule() const { return TypeDataModule; }
	const TArray<UParticleModule*>& GetModules() const { return Modules; }
	const TArray<UParticleModule*>& GetSpawnModules() const { return SpawnModules; }
	const TArray<UParticleModule*>& GetUpdateModules() const { return UpdateModules; }

	void SetRequiredModule(UParticleModuleRequired* InModule);
	void SetSpawnModule(UParticleModuleSpawn* InModule);
	void SetTypeDataModule(UParticleModuleTypeDataBase* InModule) { TypeDataModule = InModule; }
	void AddModule(UParticleModule* InModule);

	// Modules를 용도별로 나눠 SpawnModules / UpdateModules에 캐시한다.
	// 모듈을 추가·삭제, 활성화 변경한 뒤에는 다시 호출해야 한다.
	void UpdateModuleLists();

private:
	friend class UParticleSystem;

	UPROPERTY()
	int32 Level = 0;
	UPROPERTY()
	bool bEnabled = true;

	UPROPERTY()
	UParticleModuleRequired* RequiredModule = nullptr;
	UPROPERTY()
	UParticleModuleSpawn* SpawnModule = nullptr;
	UParticleModuleTypeDataBase* TypeDataModule = nullptr;   // nullptr이면 Sprite Emitter
	UPROPERTY()
	TArray<UParticleModule*>     Modules;                    // 에디터에 보이는 순서 그대로

	// 실행용 캐시 (Modules에서 파생)
	TArray<UParticleModule*> SpawnModules;
	TArray<UParticleModule*> UpdateModules;
};
