#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleEmitter.generated.h"

class UParticleLODLevel;
class UParticleSystemComponent;
struct FParticleEmitterInstance;

// Emitter
UCLASS()
class UParticleEmitter : public UObject
{
	GENERATED_BODY()
public:
	// 이 Emitter의 실행 인스턴스를 만든다. 실패하면 nullptr, TypeData에 따라 달라짐
	virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* Component);

	const TArray<UParticleLODLevel*>& GetLODLevels() const { return LODLevels; }
	UParticleLODLevel* GetLODLevel(int32 Index) const;
	void AddLODLevel(UParticleLODLevel* InLODLevel) { if (InLODLevel) LODLevels.Add(InLODLevel); }

	// 모듈 구성이 바뀐 뒤 호출. 모든 LOD의 모듈 목록을 다시 분류한다 
	// 나중에 모듈별 Payload가 생기면 ParticleSize 계산도 여기서 한다.
	void CacheEmitterModuleInfo();

private:
	TArray<UParticleLODLevel*> LODLevels;
};