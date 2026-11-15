#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "ParticleEmitter.generated.h"

class UParticleLODLevel;

UCLASS()
class UParticleEmitter : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UParticleEmitter() override;

	bool Validate() const;
	bool Build();

	// 새 LOD를 생성하고 비소유 포인터를 반환합니다.
	UParticleLODLevel* AddLODLevel();

	// 특정 LOD Level을 반환합니다.
	UParticleLODLevel* GetLODLevel(int32 Index) const;

	// 모든 LOD Level들을 반환합니다.
	const TArray<UParticleLODLevel*>& GetLODLevels() const
	{
		return LODLevels;
	}

private:
	// 설정 계층의 자식 객체입니다. 실행 Instance를 저장하지 않습니다.
	TArray<UParticleLODLevel*> LODLevels;
};
