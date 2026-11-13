#pragma once

#include "EnginePCH.h"
#include "ParticleModule.h"
#include "ParticleLODLevel.generated.h"

class UParticleModuleRequired;
class UParticleModuleTypeDataBase; // 아직 정의하지 않음. Mesh / Beam / Ribbon TypeData의 기반 클래스 자리

UCLASS()
class UParticleLODLevel : public UObject
{
	GENERATED_BODY()

public:
	const TArray<UParticleModule*>& GetModules() const { return Modules; }
	UParticleModuleRequired* GetRequiredModule() const { return RequiredModule; }
	UParticleModuleTypeDataBase* GetTypeDataModule() const { return TypeDataModule; }

private:
	int32 Level = 0;	// 현재 LOD Level
	bool bEnabled = true;
	UParticleModuleRequired* RequiredModule = nullptr;
	TArray<UParticleModule*> Modules;
	UParticleModuleTypeDataBase* TypeDataModule = nullptr;
};
