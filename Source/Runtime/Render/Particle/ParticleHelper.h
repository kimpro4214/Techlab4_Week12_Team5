#pragma once

#include "ParticleData.h"
#include <random>

// 파티클 모듈 공용 난수: [0, 1) 범위의 float 반환
// Min~Max 랜덤은 FMath::Lerp(Min, Max, ParticleRandom())으로 사용
// inline 함수 안의 static 변수라 여러 .cpp에서 include해도 생성기는 프로그램 전체에 하나만 존재
inline float ParticleRandom()
{
	static std::mt19937 Generator(std::random_device{}());
	static std::uniform_real_distribution<float> Distribution(0.0f, 1.0f);
	return Distribution(Generator);
}


#define DECLARE_PARTICLE_PTR(Name, Address) FBaseParticle* Name = (FBaseParticle*)(Address)		// 새로운 파티클 포인터 정의

// 파티클 배열 전체 순회, 해당 매크로 안에서 Particle로 접근해 Particle 전체의 값을 바꿀 수 있음
#define BEGIN_UPDATE_LOOP \
	{ \
		int32&  ActiveParticles = Owner->ActiveParticles; \
		uint8*  ParticleData    = Owner->ParticleDataBlock.ParticleData; \
		uint16* ParticleIndices = Owner->ParticleDataBlock.ParticleIndices; \
		const int32 ParticleStride = Owner->ParticleStride; \
		for (int32 i = ActiveParticles-1; i >= 0; --i) \
		{ \
			const int32 CurrentIndex = ParticleIndices[i]; \
			FBaseParticle& Particle = *((FBaseParticle*)(ParticleData + CurrentIndex * ParticleStride));
#define END_UPDATE_LOOP \
		} \
	}
