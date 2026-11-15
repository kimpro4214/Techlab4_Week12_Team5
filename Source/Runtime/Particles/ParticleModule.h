#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleModule.generated.h"

UCLASS()
class UParticleModule : public UObject
{
	GENERATED_BODY()

public:
	bool IsEnabled() const { return bEnabled; }
	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

	bool IsSpawnModule() const { return bSpawnModule; }
	bool IsUpdateModule() const { return bUpdateModule; }

	// 설정이 유효한지 체크. 자식 클래스에서 재정의함
	virtual bool Validate() const { return true; }

protected:
	bool bEnabled = true;
	bool bSpawnModule = false;
	bool bUpdateModule = false;
};

UCLASS()
class UParticleModuleRequired : public UParticleModule
{
	GENERATED_BODY()
};

UCLASS()
class UParticleModuleSpawn : public UParticleModule
{
	GENERATED_BODY()
};

UCLASS()
class UParticleModuleTypeDataBase : public UParticleModule
{
	GENERATED_BODY()
};