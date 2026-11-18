#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Containers/Map.h"
#include "ParticleEmitter.generated.h"

class UParticleLODLevel;
class UParticleSystemComponent;
class UParticleModule;
struct FParticleEmitterInstance;


// Emitter
UCLASS()
class UParticleEmitter : public UObject
{
	GENERATED_BODY()
public:
	// 이 Emitter의 실행 인스턴스를 만든다. 실패하면 nullptr, TypeData에 따라 달라짐
	virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* Component);

	const FString& GetEmitterName() const { return EmitterName; }
	void SetEmitterName(const FString& InName) { EmitterName = InName; }
	using UObject::Serialize;
	void Serialize(FArchive& Ar) override;

	const TArray<UParticleLODLevel*>& GetLODLevels() const { return LODLevels; }
	UParticleLODLevel* GetLODLevel(int32 Index) const;
	void AddLODLevel(UParticleLODLevel* InLODLevel);

	// 모듈 구성이 바뀐 뒤 호출. 모든 LOD의 모듈 목록을 다시 분류한다 
	// 나중에 모듈별 Payload가 생기면 ParticleSize 계산도 여기서 한다.
	void CacheEmitterModuleInfo(int32 LODIndex = 0);

	int32 GetParticleSize() const { return ParticleSize; } 
	int32 GetModuleOffset(const UParticleModule* Module) const { return ModuleOffsetMap.FindRef(Module); }

	uint64 GetRequiredInstanceBytes() const { return RequiredInstanceBytes; }
	const uint64* FindModuleInstanceOffset(const UParticleModule* Module) const { return ModuleInstanceOffsetMap.Find(Module); }

	bool IsLODCompatible(int32 Index) const;

private:
	friend class UParticleSystem;
	FString EmitterName;

	UPROPERTY()
	TArray<UParticleLODLevel*> LODLevels;
	int32 ParticleSize = 0;
	TMap<const UParticleModule*, int32> ModuleOffsetMap;
	uint64 RequiredInstanceBytes = 0;
	TMap<const UParticleModule*, uint64> ModuleInstanceOffsetMap;
};
