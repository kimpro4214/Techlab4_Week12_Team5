#include "EnginePCH.h"
#include "ParticleDataContainer.h"
#include "Core/HAL/UnrealMemory.h"

#include <limits>
#include <new>
#include <stdexcept>

FParticleDataContainer::~FParticleDataContainer()
{
	// 소멸시 Free 실행
	Free();
}

void FParticleDataContainer::Allocate(
	int32 InParticleDataNumBytes,
	int32 InParticleIndicesNumShorts
)
{
	// 크기는 반드시 양수를 받아야 함
	if (InParticleDataNumBytes <= 0 || InParticleIndicesNumShorts < 0)
	{
		throw std::invalid_argument("Invalid particle storage size.");
	}
	
	// 전체 메모리 블록에는 uint16인 ParticleIndices도 포함되므로, 최소 2바이트 정렬이 필요함
	if (InParticleDataNumBytes % alignof(uint16) != 0)
	{
		throw std::invalid_argument("Unaligned particle index storage.");
	}

	// 전체 크기가 오버플로우가 발생하는지 체크
	const uint64 IndexBytes = static_cast<uint64>(InParticleIndicesNumShorts) * sizeof(uint16);
	const uint64 TotalBytes = static_cast<uint64>(InParticleDataNumBytes) + IndexBytes;
	if (TotalBytes > static_cast<uint64>(std::numeric_limits<int32>::max()))
	{
		throw std::length_error("Particle storage exceeds int32 range.");
	}

	// 새 블록 확보에 실패하더라도 기존 블록은 유지
	uint8* NewParticleData = static_cast<uint8*>(FMemory::Malloc(TotalBytes));
	if (!NewParticleData)
	{
		throw std::bad_alloc();
	}

	// 이 아래는 성공이 보장되므로 기존 블록을 교체
	Free();

	MemBlockSize = static_cast<int32>(TotalBytes);

	ParticleDataNumBytes = InParticleDataNumBytes;
	ParticleIndicesNumShorts = InParticleIndicesNumShorts;

	ParticleData = NewParticleData;
	ParticleIndices = ParticleIndicesNumShorts > 0
		? reinterpret_cast<uint16*>(ParticleData + ParticleDataNumBytes)
		: nullptr;
}

void FParticleDataContainer::Free()
{
	if (ParticleData)
	{
		FMemory::Free(ParticleData);
	}

	MemBlockSize = 0;
	ParticleDataNumBytes = 0;
	ParticleIndicesNumShorts = 0;

	ParticleData = nullptr;
	ParticleIndices = nullptr;
}
