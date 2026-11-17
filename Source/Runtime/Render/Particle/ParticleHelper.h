#pragma once

#include "Render/Particle/ParticleData.h"
#include "Math/Matrix.h"
#include <random>

// 시뮬레이션 쪽 헤더, 렌더 헤더를 include 하지 않음

class UMaterial;
class UStaticMesh;

// 이미터 안의 파티클을 그리는 순서. 반투명(AlphaBlend)은 뒤,앞으로 그려야 겹침이 맞는다
// Additive는 더하는 순서와 결과가 같으므로 None으로 두면 된다
enum class EParticleSortMode : uint8
{
	None,
	ViewDepth,        // 카메라 전방축 기준 깊이. 화면에 평행한 면 단위라 직교 뷰에서도 맞다
	DistanceToView,   // 카메라 위치까지의 거리. 카메라를 둘러싼 파티클(눈, 비)에 맞다
};

enum class EDynamicEmitterType : uint8
{
	Sprite,
	Mesh,
};

// Mesh 파티클이 어느 쪽을 바라볼지
enum class EParticleMeshAlignment : uint8
{
	None,       // 시뮬레이션 공간 축 그대로, 파티클 Rotation만큼 Z축으로 돈다
	Velocity,   // 메시 X축이 진행 방향을 본다 (화살, 파편)
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
	// 컴포넌트 월드 스케일. 위치는 공간 변환에 스케일이 들어가지만 스프라이트 크기는 카메라 축으로 펼쳐서 따로 곱해야 한다
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
};

struct FDynamicSpriteEmitterReplayDataBase : public FDynamicEmitterReplayDataBase
{
	UMaterial* Material = nullptr;
	int32 MaxParticles = 0;   // GPU 버퍼 용량
	EParticleSortMode SortMode = EParticleSortMode::None;
};

struct FDynamicMeshEmitterReplayData : public FDynamicEmitterReplayDataBase
{
	UStaticMesh* Mesh = nullptr;
	EParticleMeshAlignment Alignment = EParticleMeshAlignment::None;
	// Required의 Material. ParticleMesh 재질이면 인스턴싱으로, 아니면 메시 자체 재질로 파티클마다 그린다
	UMaterial* Material = nullptr;
	int32 MaxParticles = 0;   // 인스턴스 버퍼 용량
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