#pragma once

#include "ParticleData.h"


#define DECLARE_PARTICLE_PTR(Name, Address) FBaseParticle* Name = (FBaseParticle*)(Address)		// 새로운 파티클 포인터 정의

#define BEGIN_UPDATE_LOOP \		// 파티클 배열 전체 순회, 해당 매크로 안에서 Particle로 접근해 Particle 전체의 값을 바꿀 수 있음
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
