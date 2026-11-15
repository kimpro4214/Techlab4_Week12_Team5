#include "EnginePCH.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"

UParticleEmitter::~UParticleEmitter()
{
	// 소유하는 LOD Level도 같이 해제
	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		delete LODLevel;
	}
}

bool UParticleEmitter::Validate() const
{
	// 내부 LOD Level이 잘 되어있는지 검증

	if (LODLevels.IsEmpty())
	{
		return false;
	}

	for (int32 Index = 0; Index < LODLevels.Num(); ++Index)
	{
		const UParticleLODLevel* LODLevel = LODLevels[Index];

		if (!LODLevel ||
			LODLevel->GetOuter() != this ||
			LODLevel->GetLevel() != Index ||
			!LODLevel->Validate())
		{
			return false;
		}
	}

	return true;
}

bool UParticleEmitter::Build()
{
	if (!Validate())
	{
		return false;
	}

	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (!LODLevel->BuildModuleLists())
		{
			return false;
		}
	}

	return true;
}


UParticleLODLevel* UParticleEmitter::AddLODLevel()
{
	// 새로운 LOD 레벨 추가
	UParticleLODLevel* NewLevel = NewObject<UParticleLODLevel>(this);

	// 기존 레벨 +1로 부여
	NewLevel->Level = LODLevels.Num();
	LODLevels.Add(NewLevel);

	// 기본 모듈을 초기화
	NewLevel->InitializeDefaultModules();

	return NewLevel;
}

UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 Index) const
{
	if (!LODLevels.IsValidIndex(Index))
	{
		return nullptr;
	}

	return LODLevels[Index];
}