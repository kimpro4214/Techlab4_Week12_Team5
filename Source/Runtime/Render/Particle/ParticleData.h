#pragma once

#include "EnginePCH.h"
#include "Math/Color.h"

// 파티클 1개의 시뮬레이션 상태
struct FBaseParticle
{
	FVector Location;;
	FVector Velocity;
	float RelativeTime;
	float Lifetime;
	FVector BaseVelocity;
	float Rotation;
	float RotationRate;
	FVector Size;
	FLinearColor Color;
};

// 파티클 데이터 + 인덱스를 하나의 메모리 블록으로 보관
struct FParticleDataContainer
{
	int32 MemBlockSize = 0;		// 파티클용으로 할당해놓은 전체 데이터 블럭의 크기
	int32 ParticleDataNumBytes = 0;		//  MemBlock 안에서 ParticleData 영역이 차지한 크기
	int32 ParticleIndicesNumShorts = 0;			// 현재 Index영역에 들어갈 수 있는 Index의 개수, Index의 Size와 곱하면  ParticleIndicesNumBytes 나옴
	uint8* ParticleData = nullptr; // this is also the memory block we allocated
	uint16* ParticleIndices = nullptr; // not allocated, this is at the end of the memory block
};
