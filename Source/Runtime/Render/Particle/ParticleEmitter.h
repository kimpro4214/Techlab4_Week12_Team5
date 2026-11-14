#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleEmitter.generated.h"

class UParticleLODLevel;

// Emitter 템플릿 (UE: UParticleEmitter)
UCLASS()
class UParticleEmitter : public UObject
{
	GENERATED_BODY()
public:
	const TArray<UParticleLODLevel*>& GetLODLevels() const { return LODLevels; }
	UParticleLODLevel* GetLODLevel(int32 Index) const;
	void AddLODLevel(UParticleLODLevel* InLODLevel) { if (InLODLevel) LODLevels.Add(InLODLevel); }

	// 모듈 구성이 바뀐 뒤 호출. 모든 LOD의 모듈 목록을 다시 분류한다 
	// 나중에 모듈별 Payload가 생기면 ParticleSize 계산도 여기서 한다.
	void CacheEmitterModuleInfo();

private:
	TArray<UParticleLODLevel*> LODLevels;
};