#pragma once

#include "Core/Types.h"

struct FParticleDataContainer
{

	FParticleDataContainer() = default;
	~FParticleDataContainer();

	// 복사 생성/대입 금지
	FParticleDataContainer(const FParticleDataContainer&) = delete;
	FParticleDataContainer& operator=(const FParticleDataContainer&) = delete;

	// 전체 메모리 블록의 크기
	int32 MemBlockSize = 0;

	// ParticleData 블록의 크기 (인덱스 블록 제외)
	int32 ParticleDataNumBytes = 0;

	// ParticleIndices의 인덱스 갯수
	int32 ParticleIndicesNumShorts = 0;
	
	// 파티클 블록 시작 주소
	uint8* ParticleData = nullptr;

	// 파티클 인덱스 블록 시작 주소
	uint16* ParticleIndices = nullptr;

	void Allocate(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts);
	void Free();
};
