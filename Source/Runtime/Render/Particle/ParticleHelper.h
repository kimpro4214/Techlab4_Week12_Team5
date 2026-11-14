#pragma once

#include "Math/Color.h"
#include <random>

// 파티클 1개의 시뮬레이션 상태 
// Base 는 Spawn 모듈이 정한 "기준값". 
// 현재값(Velocity, Size)은 매 프레임 기준값으로 되돌린 뒤 Update 모듈이 다시 계산한다.

struct FBaseParticle
{
	FVector Location;
	FVector Velocity;
	FVector BaseVelocity;
	float RelativeTime = 0.0f;
	float LifeTime = 0.0f;
	float Rotation = 0.0f;
	float RotationRate = 0.0f;
	FVector Size = FVector(1.0f, 1.0f, 1.0f);
	FVector BaseSize = FVector(1.0f, 1.0f, 1.0f);
	FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	FLinearColor BaseColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};

enum class EParticleSpawnType
{
	None,
	DistanceToView, // 카메라에서 먼 것부터 (Back-to-Front) 
};

// inline 함수 안의 static이라 여러 .cpp에서 include 해도 생성기는 하나만 존재하낟
inline float ParticleRandom()
{
	static std::mt19937 Generator(std::random_device{}());
	static std::uniform_real_distribution<float> Distribution(0.0f, 1.0f);
	return Distribution(Generator);
}

inline float ParticleRandRange(float Min, float Max)
{
	return Min + (Max - Min) * ParticleRandom();
}

inline FVector ParticleRandRange(const FVector& Min, const FVector& Max)
{
	return FVector(ParticleRandRange(Min.X, Max.X), ParticleRandRange(Min.Y, Max.Y), ParticleRandRange(Min.Z, Max.Z));
}


// ── 파티클 순회 매크로 (UE: BEGIN_UPDATE_LOOP / END_UPDATE_LOOP) ──
// 모듈의 Update는 반드시 이 매크로 안에서 Particle로만 접근한다.
// 지금은 TArray<FBaseParticle> 기반이지만, 나중에 ParticleData/ParticleIndices(Payload) 블록으로
// 바꿀 때 이 매크로만 고치면 모듈 코드는 그대로 둘 수 있다.
// 뒤에서부터 도는 이유: 루프 중 KillParticle(swap-remove)을 해도 안전하다.
#define BEGIN_UPDATE_LOOP \
	{ \
		TArray<FBaseParticle>& ParticleArray = Owner->Particles; \
		for (int32 ParticleIdx = ParticleArray.Num() - 1; ParticleIdx >= 0; --ParticleIdx) \
		{ \
			FBaseParticle& Particle = ParticleArray[ParticleIdx];

#define END_UPDATE_LOOP \
		} \
	}