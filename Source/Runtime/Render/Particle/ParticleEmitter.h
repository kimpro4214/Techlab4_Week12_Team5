#pragma once

#include "EnginePCH.h"
#include "ParticleLODLevel.h"
#include "ParticleEmitter.generated.h"

UCLASS()
class UParticleEmitter : public UObject
{
	GENERATED_BODY()

public:
	const TArray<UParticleLODLevel*>& GetLODLevels() const { return LODLevels; }
	void AddLODLevels(UParticleLODLevel* InLODLevel) { if (InLODLevel) LODLevels.Add(InLODLevel); }

private:
	TArray<UParticleLODLevel*> LODLevels;
};
