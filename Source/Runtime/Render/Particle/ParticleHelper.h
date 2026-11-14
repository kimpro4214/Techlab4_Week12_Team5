#pragma once

#include "Math/Color.h"
#include <random>

// 시뮬레이션 쪽 헤더, 렌더 헤더를 include 하지 않음

enum class EParticleSpawnType
{
	None,
	DistanceToView, // 카메라에서 먼 것부터 (Back-to-Front) 
};

enum class EDynamicEmitterType : uint8
{
	Sprite,
	Mesh,
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


// 블록 안의 주소를 파티클 포인터로 해석한다
#define DECLARE_PARTICLE_PTR(Name, Address) FBaseParticle* Name = reinterpret_cast<FBaseParticle*>(Address)


// --파티클 순회 매크로--
// 살아있는 파티클을 순회한다. 스코프에 Owner(FParticleEmitterInstance*)가 있어야 한다.
// 실제 데이터 위치 = ParticleData + (칸 번호 × Stride), 칸 번호 = ParticleIndices[i]
// 뒤에서부터 도는 이유: 루프 중 KillParticle(swap-remove)을 해도 안전하다.
#define BEGIN_UPDATE_LOOP \
	{ \
		uint8*        ParticleData    = Owner->ParticleDataBlock.ParticleData; \
		const uint16* ParticleIndices = Owner->ParticleDataBlock.ParticleIndices; \
		const int32   ParticleStride  = Owner->ParticleStride; \
		for (int32 i = Owner->ActiveParticles - 1; i >= 0; --i) \
		{ \
			const int32 CurrentIndex = ParticleIndices[i]; \
			FBaseParticle& Particle = *reinterpret_cast<FBaseParticle*>(ParticleData + CurrentIndex * ParticleStride);

#define END_UPDATE_LOOP \
		} \
	}