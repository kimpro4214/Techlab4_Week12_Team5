#pragma once

#include "EnginePCH.h"
#include "ParticleHelper.h"
#include "ParticleEmitter.h"
#include "Core/HAL/UnrealMemory.h"

class UParticleSystemComponent;

// Component × Emitter마다 1개씩 존재하는 Emitter의 실행 상태
// 파티클 메모리를 보유하고 Spawn / Update / Kill을 수행
struct FParticleEmitterInstance
{
	bool Init(UParticleEmitter* Emitter, UParticleSystemComponent* ParticleComponent);

	void Spawn(int32 LODIndex)
	{
		const TArray<UParticleModule*> Modules = SpriteTemplate->GetLODLevels()[LODIndex]->GetModules();
		UParticleModuleRequired* RequiredModule = SpriteTemplate->GetLODLevels()[LODIndex]->GetRequiredModule();
		UParticleModuleTypeDataBase* TypeDataModule = SpriteTemplate->GetLODLevels()[LODIndex]->GetTypeDataModule();

		for (int i = 0;i < Modules.Num();i++)
		{
			Modules[i]->Spawn();
		}
	}

	void Update()
	{

	}

	UParticleEmitter* SpriteTemplate = nullptr;	// 실제 Emitter 참조로 Module 가져옴
	UParticleSystemComponent* Component = nullptr;	// Component에 정보를 전달하기 위한 참조
	UParticleLODLevel* CurrentLODLevel = nullptr;	// 현재 Emitter의 LODLevel
	FParticleDataContainer ParticleDataBlock;	// 파티클 데이터를 담는 Block
	int32 ParticleSize;	// Particle 자체의 사이즈, 
	int32 ParticleStride;	// Particle Size가 16단위가 아닐 경우 맞추기 위한 간격, ex) Size=84 -> Stride=96
	int32 ActiveParticles; // 활성화된 파티클 개수
	int32 MaxActiveParticles;	// 가능한 파티클 최대 개수
	float SpawnFraction;	//다음 프레임으로 넘길 생성량 소수점
};
