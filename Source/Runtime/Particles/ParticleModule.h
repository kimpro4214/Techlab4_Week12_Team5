#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleHelper.h"
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

	// 필요한 데이터 크기
	virtual int32 RequiredBytes() const { return 0; }
	virtual int32 RequiredBytesPerInstance() const { return 0; }
	
	// 정렬은 2의 거듭제곱
	virtual int32 GetParticlePayloadAlignment() const { return 1; }
	virtual int32 GetInstancePayloadAlignment() const { return 1; }

	// 생성과 Reset에서 호출합니다. Payload는 별도 소멸 처리 없이 재사용합니다.
	virtual void InitializeInstance(const FParticleInstanceContext& Context) const noexcept {}
	virtual void Spawn(const FParticleSpawnContext& Context) const noexcept {}
	virtual void Update(const FParticleUpdateContext& Context) const noexcept {}

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
