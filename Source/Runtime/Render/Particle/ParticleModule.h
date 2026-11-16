#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleModule.generated.h"

struct FParticleEmitterInstance;
struct FBaseParticle;

class UParticleModuleTypeDataBase;

// 모든 파티클 모듈의 기반 
UCLASS()
class UParticleModule : public UObject
{
	GENERATED_BODY()
public:
	struct FSpawnContext
	{
		FParticleEmitterInstance& Owner;
		int32 Offset;
		float SpawnTime;
		FBaseParticle* ParticleBase;
	};

	struct FUpdateContext
	{
		FParticleEmitterInstance& Owner;
		int32 Offset;
		float DeltaTime;
	};
	// 파티클이 생성될 때 한 번 호출된다.
	//   Offset    : Payload 안에서 이 모듈 전용 데이터가 시작하는 위치
	//   SpawnTime : 이 파티클이 이번 프레임 안에서 이미 살아있던 시간
	virtual void Spawn(const FSpawnContext& Context) {}

	// 매 프레임 살아있는 모든 파티클에 대해 호출된다. 
	virtual void Update(const FUpdateContext& Context) {}

	// LODLevel이 SpawnModules / UpdateModules를 분류할 때 쓴다. 하위 모듈이 생성자에서 킨다.
	bool IsSpawnModule() const { return bSpawnModule; }
	bool IsUpdateModule() const { return bUpdateModule; }
	bool IsEnabled() const { return bEnabled; }
	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

	virtual uint32 RequiredBytes(const UParticleModuleTypeDataBase* TypeData) const { return 0; }
	virtual uint32 GetParticlePayloadAlignment() const { return 1; }
	virtual uint32 RequiredBytesPerInstance() const { return 0; }
	virtual uint32 GetInstancePayloadAlignment() const { return 1; }

	virtual void PrepPerInstanceBlock(FParticleEmitterInstance& Owner, void* InstanceData) {}
protected:
	bool bSpawnModule = false;
	bool bUpdateModule = false;
	bool bEnabled = true;
};