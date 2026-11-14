#pragma once

#include "Math/Color.h"

// 파티클 1개의 시뮬레이션 상태 
// Base 는 Spawn 모듈이 정한 "기준값". 
// 현재값(Velocity, Size)은 매 프레임 기준값으로 되돌린 뒤 Update 모듈이 다시 계산한다.
// Payload 블록에서 한 칸(ParticleStride)의 맨 앞을 차지한다.
// 생성 시 칸 전체를 Memzero한 뒤 PreSpawn과 모듈이 값을 채우므로 기본값 초기화를 두지 않는다.
struct FBaseParticle
{
	FVector Location;
	FVector Velocity;  // 현재 속도. 매 프레임 BaseVelocity로 되돌린 뒤 Update 모듈이 가공한다
	FVector BaseVelocity; // Spawn 모듈이 정한 기준 속도
	float RelativeTime = 0.0f; // 0~1. 수명 진행도. 매 프레임 Update에서 증가한다
	float LifeTime = 0.0f; // 1 / 수명(초). 매 프레임 나눗셈을 피하려고 미리 나눠 둔다
	float Rotation = 0.0f;
	float RotationRate = 0.0f;
	FVector Size = FVector(1.0f, 1.0f, 1.0f);
	FVector BaseSize = FVector(1.0f, 1.0f, 1.0f); // Spawn 모듈이 정한 기준 크기
	FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	FLinearColor BaseColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};

// 파티클 데이터 + 인덱스를 한 번의 할당으로 보관하는 블록
// ParticleIndices[0 .. ActiveParticles) = 살아있는 파티클이 들어 있는 "칸 번호"
struct FParticleDataContainer
{
	int32 MemBlockSize = 0; // Payload 블록 전체 크기. ParticleStride * MaxParticles
	int32 ParticleDataNumBytes = 0; // 파티클 데이터 영역 바이트 (Stride * MaxParticles)
	int32 ParticleIndicesNumShorts = 0; // 인덱스 개수 (= MAX)
	uint8* ParticleData = nullptr; // 블록 시작 주소
	uint16* ParticleIndices = nullptr; // 파티클 인덱스 배열. 
};