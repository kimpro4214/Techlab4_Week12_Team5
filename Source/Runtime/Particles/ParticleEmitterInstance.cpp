#include "EnginePCH.h"
#include "ParticleEmitterInstance.h"
#include "BaseParticle.h"
#include "ParticleDataContainer.h"

#include <cassert>
#include <cmath>
#include <new>
#include <utility>

FParticleEmitterInstance::FParticleEmitterInstance() = default;

FParticleEmitterInstance::~FParticleEmitterInstance()
{
	if (Container)
	{
		Container->Free();
	}
}

void FParticleEmitterInstance::Initialize(
	UParticleEmitter* InSpriteTemplate,
	UParticleSystemComponent* InComponent,
	int32 InCurrentLODLevelIndex,
	UParticleLODLevel* InCurrentLODLevel,
	int32 InCapacity,
	float InDefaultLifetime
)
{
	// 범위가 0 ~ 65535 이내인가?
	assert(InCapacity > 0 && InCapacity <= 65535);

	// InDefaultLifetime 유효 체크 및 0보다 큰지
	assert(std::isfinite(InDefaultLifetime) && InDefaultLifetime > 0.0f);

	// 컨테이너 없으면 만들기
	if (Container)
	{
		Container->Free();
	}
	else
	{
		Container = MakeUnique<FParticleDataContainer>();
	}

	// 인자 넣기
	SpriteTemplate = InSpriteTemplate;
	Component = InComponent;
	CurrentLODLevelIndex = InCurrentLODLevelIndex;
	CurrentLODLevel = InCurrentLODLevel;
	DefaultLifetime = InDefaultLifetime;

	// 기본값 초기화
	PayloadOffset = sizeof(FBaseParticle);
	ParticleSize = sizeof(FBaseParticle);
	ParticleStride = sizeof(FBaseParticle);
	InstanceData = nullptr;
	InstancePayloadSize = 0;
	ActiveParticles = 0;
	ParticleCounter = 0;
	MaxActiveParticles = 0;

	// Container를 기본 값으로 초기화
	Container->Allocate(ParticleStride * InCapacity, InCapacity);
	ParticleData = Container->ParticleData;
	ParticleIndices = Container->ParticleIndices;

	// 제대로 초기화 되었는가?
	assert(ParticleData);
	assert(ParticleIndices);

	// 최대 활성 입자 수를 설정
	MaxActiveParticles = InCapacity;

	// 인덱스 목록 초기화
	for (int32 Index = 0; Index < MaxActiveParticles; ++Index)
	{
		ParticleIndices[Index] = static_cast<uint16>(Index);
	}
}

void FParticleEmitterInstance::SpawnParticles(
	int32 Count,
	float StartTime,
	float Increment,
	const FVector& InitialLocation,
	const FVector& InitialVelocity,
	FParticleEventInstancePayload* EventPayload
)
{
	// 0개 이하의 파티클을 생성하려고 함
	if (Count <= 0) { return; }

	// 아직 파티클 메모리 블록이 없음
	if (!ParticleData) { return; }

	// StartTime, Increment가 유한한 값인지 확인
	if (!std::isfinite(StartTime)) { return; }
	if (!std::isfinite(Increment)) { return; }

	// StartTime과 Increment가 양수인지 확인
	if (StartTime < 0.0f) { return; }
	if (Increment < 0.0f) { return; }

	// TODO: 이벤트 Payload의 형식과 배치는 아직 지원하지 않음
	(void)EventPayload;

	// MaxActiveParticles 갯수까지만 생성.
	// TODO: 추후 메모리 블록을 늘리는 기능을 넣어야함
	const int32 SpawnCount = std::min(Count, MaxActiveParticles - ActiveParticles);

	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const uint16 Slot = ParticleIndices[ActiveParticles];
		uint8* SlotData = ParticleData + Slot * ParticleStride;

		// 재사용 슬롯에서도 모든 필드를 초기화하고 객체 수명을 시작합니다.
		FBaseParticle* Particle = new (SlotData) FBaseParticle{};
		const double Elapsed = static_cast<double>(StartTime)
			- static_cast<double>(Index) * Increment;
		const float Age = Elapsed > 0.0 ? static_cast<float>(Elapsed) : 0.0f;

		Particle->OldLocation = InitialLocation;
		Particle->Location = InitialLocation + InitialVelocity * Age;
		Particle->BaseVelocity = InitialVelocity;
		Particle->Velocity = InitialVelocity;
		Particle->BaseSize = FVector(1.0f);
		Particle->Size = Particle->BaseSize;
		Particle->BaseColor = FColor(255, 255, 255, 255);
		Particle->Color = Particle->BaseColor;
		Particle->Lifetime = DefaultLifetime;
		Particle->RelativeTime = Age / Particle->Lifetime;

		++ActiveParticles;
		++ParticleCounter;
	}
}

void FParticleEmitterInstance::KillParticle(int32 Index)
{
	if (Index < 0 || Index >= ActiveParticles)
	{
		return;
	}

	// 반환된 슬롯은 비활성 구간의 맨 앞에 놓여 다음 생성에서 재사용됩니다.
	--ActiveParticles;
	std::swap(ParticleIndices[Index], ParticleIndices[ActiveParticles]);
}

void FParticleEmitterInstance::KillParticles()
{
	// 뒤에서 순회하면 교환되어 들어온 입자를 다시 검사할 필요가 없습니다.
	for (int32 Index = ActiveParticles - 1; Index >= 0; --Index)
	{
		if (GetParticle(Index)->RelativeTime >= 1.0f)
		{
			KillParticle(Index);
		}
	}
}

FBaseParticle* FParticleEmitterInstance::GetParticle(int32 Index)
{
	return const_cast<FBaseParticle*>(
		static_cast<const FParticleEmitterInstance&>(*this).GetParticle(Index));
}

const FBaseParticle* FParticleEmitterInstance::GetParticle(int32 Index) const
{
	const int32 Slot = GetParticleSlot(Index);
	return Slot < 0 ? nullptr
		: reinterpret_cast<const FBaseParticle*>(ParticleData + Slot * ParticleStride);
}

int32 FParticleEmitterInstance::GetParticleSlot(int32 Index) const
{
	return Index >= 0 && Index < ActiveParticles ? ParticleIndices[Index] : -1;
}
