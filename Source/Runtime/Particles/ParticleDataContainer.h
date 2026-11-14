#pragma once

#include "Core/Types.h"

struct FParticleDataContainer
{
	// 전체 메모리 블록의 크기
	int32 MemBlockSize = 0;

	// 입자 데이터 하나당 필요한 바이트 수
	int32 ParticleDataNumBytes = 0;

	// 입자 데이터 갯수
	int32 ParticleIndicesNumShorts = 0;

	// 입자 데이터 시작 주소
	uint8* ParticleData = nullptr;

	// 입자 데이터 인덱스 시작 주소
	uint16* ParticleIndices = nullptr;

	void Allocate(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts);
	void Free();
};
