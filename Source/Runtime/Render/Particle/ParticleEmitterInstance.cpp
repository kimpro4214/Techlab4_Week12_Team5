#include "EnginePCH.h"
#include "Render/Particle/ParticleEmitterInstance.h"
#include "Render/Particle/ParticleEmitter.h"
#include "Render/Particle/ParticleLODLevel.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleSpawn.h"
#include "Core/HAL/UnrealMemory.h"
#include <algorithm>

FParticleEmitterInstance::~FParticleEmitterInstance()
{
	FreeParticleData();
}

bool FParticleEmitterInstance::Init(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent)
{
	if (!InTemplate || !InComponent)
		return false;

	SpriteTemplate = InTemplate;
	Component = InComponent;
	CurrentLODLevelIndex = 0;
	CurrentLODLevel = SpriteTemplate->GetLODLevel(CurrentLODLevelIndex);
	if (!CurrentLODLevel)
		return false;

	// Required·Spawn이 없으면 블록 크기도 생성량도 알 수 없으므로, 할당 전에 실패시킨다
	UParticleModuleRequired* Required = CurrentLODLevel->GetRequiredModule();
	if (!Required || !CurrentLODLevel->GetSpawnModule())
		return false;

	// 칸 크기: 지금은 FBaseParticle뿐. 모듈별 Payload가 생기면 여기에 더해진다.
	ParticleSize = sizeof(FBaseParticle);
	// 칸 간격을 16의 배수로 올린다 → 모든 칸의 시작 주소가 16바이트 정렬 (Address Align)
	ParticleStride = static_cast<int32>(FMemory::QuantizeSize(ParticleSize, 16));

	ActiveParticles = 0;
	ParticleCounter = 0;
	SpawnFraction = 0.0f;
	EmitterTime = 0.0f;

	const int32 MaxParticles = std::clamp(Required->GetMaxParticles(), 1, 65535);
	return AllocateParticleData(MaxParticles);
}

bool FParticleEmitterInstance::AllocateParticleData(int32 MaxParticles)
{
	FreeParticleData();
	MaxActiveParticles = MaxParticles;

	// [ 파티클 데이터 (Stride × Max) | 인덱스 (uint16 × Max) ] 를 한 번에 할당 
	// 실행 중에는 더 할당하지 않는다.
	ParticleDataBlock.ParticleDataNumBytes = ParticleStride * MaxParticles;
	ParticleDataBlock.ParticleIndicesNumShorts = MaxParticles;
	ParticleDataBlock.MemBlockSize =
		ParticleDataBlock.ParticleDataNumBytes + ParticleDataBlock.ParticleIndicesNumShorts * static_cast<int32>(sizeof(uint16));

	ParticleDataBlock.ParticleData = static_cast<uint8*>(FMemory::Malloc(ParticleDataBlock.MemBlockSize));
	if (!ParticleDataBlock.ParticleData)
		return false;

	// 인덱스 영역은 데이터 영역 바로 뒤
	ParticleDataBlock.ParticleIndices =
		reinterpret_cast<uint16*>(ParticleDataBlock.ParticleData + ParticleDataBlock.ParticleDataNumBytes);

	// 처음엔 칸 i에 인덱스 i. Spawn/Kill이 이 순서를 섞는다
	for (int32 i = 0; i < MaxParticles; ++i)
		ParticleDataBlock.ParticleIndices[i] = static_cast<uint16>(i);

	return true;
}

void FParticleEmitterInstance::FreeParticleData()
{
	// ParticleIndices는 같은 블록 안이므로 ParticleData 하나만 해제한다
	if (ParticleDataBlock.ParticleData)
		FMemory::Free(ParticleDataBlock.ParticleData);

	ParticleDataBlock = FParticleDataContainer{};
	ActiveParticles = 0;
	MaxActiveParticles = 0;
}