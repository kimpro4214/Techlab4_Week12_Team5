#include "EnginePCH.h"
#include "Render/Particle/ParticleEmitterInstance.h"

FParticleEmitterInstance::~FParticleEmitterInstance()
{
	// ParticleData가 블록 전체의 시작 주소. ParticleIndices는 같은 블록 안의 위치라 따로 해제하지 않음
	if (ParticleDataBlock.ParticleData != nullptr)
	{
		FMemory::Free(ParticleDataBlock.ParticleData);
	}
	ParticleDataBlock.ParticleData = nullptr;
	ParticleDataBlock.ParticleIndices = nullptr;
}

bool FParticleEmitterInstance::Init(UParticleEmitter* Emitter, UParticleSystemComponent* ParticleComponent)
{
	if (Emitter != nullptr)		SpriteTemplate = Emitter;	else return false;
	if (ParticleComponent != nullptr)		Component = ParticleComponent;	else return false;

	if (SpriteTemplate->GetLODLevels().Num() == 0) return false;
	else CurrentLODLevel = SpriteTemplate->GetLODLevels()[0];

	ParticleSize = sizeof(FBaseParticle);
	ParticleStride = FMemory::QuantizeSize(ParticleSize, 16);		// ParticleSize 바탕으로 16배수로 맞춤

	ActiveParticles = 0;
	MaxActiveParticles = 100;
	SpawnFraction = 0;

	ParticleDataBlock.ParticleDataNumBytes = ParticleStride * MaxActiveParticles;
	ParticleDataBlock.ParticleIndicesNumShorts = MaxActiveParticles;
	ParticleDataBlock.MemBlockSize = ParticleDataBlock.ParticleDataNumBytes + ParticleDataBlock.ParticleIndicesNumShorts * sizeof(uint16);	// Block 전체 Size = Particle 영역 + Index 영역

	ParticleDataBlock.ParticleData = (uint8*)FMemory::Malloc(ParticleDataBlock.MemBlockSize);	// 블럭 전체 메모리 할당
	if (ParticleDataBlock.ParticleData == nullptr) return false;
	ParticleDataBlock.ParticleIndices = (uint16*)(ParticleDataBlock.ParticleData + ParticleDataBlock.ParticleDataNumBytes);	// Indices 영역 시작지점
	if (ParticleDataBlock.ParticleIndices == nullptr) return false;

	for (int i = 0;i < MaxActiveParticles;i++)
	{
		ParticleDataBlock.ParticleIndices[i] = i;	// Indices 배열에 Indices 채움
	}

	return true;
}

FParticleEmitterInstance::~FParticleEmitterInstance()
{
	if (ParticleDataBlock.ParticleData != nullptr)
	{
		FMemory::Free(ParticleDataBlock.ParticleData);
	}
	ParticleDataBlock.ParticleData =  nullptr;
	ParticleDataBlock.ParticleIndices = nullptr;
}