#pragma once

#include "Render/Particle/ParticleData.h"
#include "Math/Matrix.h"
#include <random>

// 시뮬레이션 쪽 헤더, 렌더 헤더를 include 하지 않음

class UMaterial;

enum class EParticleSpawnType
{
	None,
	DistanceToView, // 카메라에서 먼 것부터 (Back-to-Front) 
};

enum class EDynamicEmitterType : uint8
{
	Sprite,
	Mesh,
	Ribbon,
	Beam
};

// 렌더 쪽이 읽을 이미터 스냅샷 
// UE는 렌더 스레드가 따로 있어서 파티클 블록을 memcpy한다. 우리는 싱글 스레드라 포인터로 가리키기만 한다
struct FDynamicEmitterReplayDataBase
{
	EDynamicEmitterType eEmitterType = EDynamicEmitterType::Sprite;
	int32 ActiveParticleCount = 0;
	int32 ParticleStride = 0;
	const uint8*  ParticleData    = nullptr;   // EmitterInstance 블록을 가리킨다 (비소유)
	const uint16* ParticleIndices = nullptr;
	FMatrix SimulationToWorld = FMatrix::Identity;
	FVector EmitterWorldOrigin = FVector::ZeroVector;   // 반투명 패킷 정렬(이미터끼리 순서)용
};

struct FDynamicSpriteEmitterReplayDataBase : public FDynamicEmitterReplayDataBase
{
	UMaterial* Material = nullptr;
	int32 MaxParticles = 0;   // GPU 버퍼 용량
};

// 띠(Beam / Ribbon)를 이루는 점 하나
// 좌표는 시뮬레이션 공간이다. 월드 변환은 부모의 SimulationToWorld로 한다 (Sprite와 같은 규칙)
struct FParticleStripPoint
{
	FVector      Position = FVector::ZeroVector;   // 띠 중심선 위의 점
	float        Width = 1.0f;                     // 이 점에서의 띠 폭
	FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};

struct FDynamicStripEmitterReplayDataBase : public FDynamicEmitterReplayDataBase
{
	UMaterial* Material = nullptr;
	int32 MaxPoints = 0;                     // 렌더 쪽 버퍼 용량 (한 프레임에 나올 수 있는 최대 점 수)
	TArray<FParticleStripPoint> Points;      // 모든 줄의 점을 순서대로 이어 붙인 것
	TArray<int32> StripPointCounts;          // 한 띠에 해당하는 점 개수. 합계 = Points.Num()
};

// inline 함수 안의 static이라 여러 .cpp에서 include 해도 생성기는 하나만 존재한다
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