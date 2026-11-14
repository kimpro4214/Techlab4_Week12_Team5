#include "EnginePCH.h"
#include "ParticleDataContainer.h"
#include "Core/HAL/UnrealMemory.h"

#include <cassert>

void FParticleDataContainer::Allocate(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts)
{
	// 크기는 반드시 양수를 받아야 함
	assert(InParticleDataNumBytes > 0 && InParticleIndicesNumShorts >= 0);
	
	// 전체 메모리 블록에는 uint16인 ParticleIndices도 포함되므로, 최소 2바이트 정렬이 필요함
	assert(InParticleDataNumBytes % sizeof(uint16) == 0);

	ParticleDataNumBytes = InParticleDataNumBytes;
	ParticleIndicesNumShorts = InParticleIndicesNumShorts;

	MemBlockSize = ParticleDataNumBytes + ParticleIndicesNumShorts * sizeof(uint16);

	ParticleData = static_cast<uint8*>(FMemory::Malloc(MemBlockSize));
	ParticleIndices = ParticleData
		? reinterpret_cast<uint16*>(ParticleData + ParticleDataNumBytes)
		: nullptr;
}

void FParticleDataContainer::Free()
{
	if (ParticleData)
	{
		// 전체 메모리 크기가 0보다 커야함
		assert(MemBlockSize > 0);
		FMemory::Free(ParticleData);
	}

	MemBlockSize = 0;
	ParticleDataNumBytes = 0;
	ParticleIndicesNumShorts = 0;
	ParticleData = nullptr;
	ParticleIndices = nullptr;
}
