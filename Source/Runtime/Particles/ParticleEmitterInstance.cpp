#include "EnginePCH.h"
#include "ParticleEmitterInstance.h"
#include "BaseParticle.h"
#include "ParticleDataContainer.h"

#include <cassert>
#include <cmath>
#include <new>
#include <utility>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>

FParticleEmitterInstance::FParticleEmitterInstance() = default;
FParticleEmitterInstance::~FParticleEmitterInstance() = default;

void FParticleEmitterInstance::Initialize(
	UParticleEmitter* InSpriteTemplate,
	UParticleSystemComponent* InComponent,
	int32 InCurrentLODLevelIndex,
	UParticleLODLevel* InCurrentLODLevel,
	int32 InCapacity,
	float InDefaultLifetime
)
{
	// InDefaultLifetime 유효 체크 및 0보다 큰지
	if (!std::isfinite(InDefaultLifetime) || InDefaultLifetime <= 0.0f)
	{
		throw std::invalid_argument("Invalid particle lifetime.");
	}

	// LOD Index는 반드시 0 이상
	if (InCurrentLODLevelIndex < 0)
	{
		throw std::invalid_argument("Invalid particle LOD index.");
	}

	// FBaseParticle가 특수한 소멸 처리(ex. 메모리 할당 해제)가 없는 default 소멸자만 허용
	static_assert(std::is_trivially_destructible_v<FBaseParticle>);

	// FBaseParticle의 정렬이 일반적인 최대 정렬 바이트와 같거나 작아야함
	static_assert(alignof(FBaseParticle) <= alignof(std::max_align_t));

	// 초기 파티클 메모리 블록 크기는 FBaseParticle의 크기로 지정
	const int32 BaseParticleSize = static_cast<int32>(sizeof(FBaseParticle));
	InitializeParticleStorage(BaseParticleSize, InCapacity);

	// 인자 넣기
	SpriteTemplate = InSpriteTemplate;
	Component = InComponent;

	CurrentLODLevelIndex = InCurrentLODLevelIndex;
	CurrentLODLevel = InCurrentLODLevel;
	DefaultLifetime = InDefaultLifetime;

	// 기본값 초기화
	ParticleSize = sizeof(FBaseParticle);
	PayloadOffset = sizeof(FBaseParticle);

	InstanceData = nullptr;
	InstancePayloadSize = 0;
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

void FParticleEmitterInstance::KillAllParticles()
{
	// 굳이 메모리를 번거롭게 0으로 초기화하면 오버헤드가 발생
	// 그냥 모든 공간이 비어있다고 표시
	ActiveParticles = 0;
}

void FParticleEmitterInstance::Reset()
{
	KillAllParticles();

	// Reset은 슬롯 사용 순서도 초기 상태로 되돌림
	for (int32 Index = 0; Index < MaxActiveParticles; ++Index)
	{
		ParticleIndices[Index] = static_cast<uint16>(Index);
	}

	// 카운터도 초기화
	ParticleCounter = 0;
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

void FParticleEmitterInstance::InitializeParticleStorage(
	int32 InParticleStride,
	int32 InCapacity
)
{
	// 범위가 0 ~ 65535 이내인가?
	if (InCapacity <= 0 || InCapacity > 65535)
	{
		throw std::invalid_argument("Invalid particle capacity.");
	}

	// 적어도 Stride가 FBaseParticle의 크기 이상이어야 함
	if (InParticleStride < static_cast<int32>(sizeof(FBaseParticle)))
	{
		throw std::invalid_argument("Invalid particle stride.");
	}

	// 적어도 Stride가 FBaseParticle의 정렬에 맞춰져야 함
	if (InParticleStride % alignof(FBaseParticle) != 0)
	{
		throw std::invalid_argument("Invalid particle stride.");
	}

	// 전체 메모리 블록 크기
	const uint64 ParticleBytes = static_cast<uint64>(InParticleStride) * static_cast<uint64>(InCapacity);

	// 메모리 블록 크기가 int32 양수 영역에 들어가는가
	if (ParticleBytes > static_cast<uint64>(std::numeric_limits<int32>::max()))
	{
		throw std::length_error("Particle data exceeds int32 range.");
	}

	// 새 저장소가 완성될 때까지 기존 저장소와 실행 상태는 유지
	auto NewContainer = MakeUnique<FParticleDataContainer>();
	NewContainer->Allocate(static_cast<int32>(ParticleBytes), InCapacity);

	for (int32 Index = 0; Index < InCapacity; ++Index)
	{
		NewContainer->ParticleIndices[Index] = static_cast<uint16>(Index);
	}

	// 컨테이너를 서로 스왑.
	// 기존 컨테이너는 NewContainer에 담기게 되어 함수 종료시 사라진다.
	Container.swap(NewContainer);

	ParticleData = Container->ParticleData;
	ParticleIndices = Container->ParticleIndices;

	ParticleStride = InParticleStride;
	MaxActiveParticles = InCapacity;

	ActiveParticles = 0;
	ParticleCounter = 0;
}
