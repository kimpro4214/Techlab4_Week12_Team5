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

enum class EDynamicEmitterType : uint8
{
	Sprite,
	Mesh,
};

// 렌더링에 필요한 Emitter 상태의 스냅샷 (원본 데이터)
struct FDynamicEmitterReplayDataBase
{
	EDynamicEmitterType eEmitterType = EDynamicEmitterType::Sprite;
	int32 ActiveParticleCount = 0;

	// UE는 FParticleDataContainer(바이트 블록)를 memcpy한다.
	// Payload 도입 전까지는 배열 복사로 대신한다.
	// TODO : 교체

	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
	FVector Origin = FVector::ZeroVector;   // 컴포넌트 위치. Emitter 단위 반투명 정렬 키로 쓴다
	int32   SortMode = 0;                   // EParticleSortMode
	// 파티클(입자)들을 화면에 그릴 때 어떤 순서로 정렬할지 결정하는 열거형(Enum) 타입
};

// 스프라이트 Emitter 용
struct FDynamicSpriteEmitterReplayDataBase : public FDynamicEmitterReplayDataBase
{
	UMaterial* Material = nullptr;
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


// --파티클 순회 매크로--
// 모듈의 Update는 반드시 이 매크로 안에서 Particle로만 접근한다..
// 뒤에서부터 도는 이유: 루프 중 KillParticle(swap-remove)을 해도 안전하다.
// TODO : ParticleData/ParticleIndices(Payload)로 변경 시에 수정요망
#define BEGIN_UPDATE_LOOP \
	{ \
		TArray<FBaseParticle>& ParticleArray = Owner->Particles; \
		for (int32 ParticleIdx = ParticleArray.Num() - 1; ParticleIdx >= 0; --ParticleIdx) \
		{ \
			FBaseParticle& Particle = ParticleArray[ParticleIdx];

#define END_UPDATE_LOOP \
		} \
	}