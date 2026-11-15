#pragma once

#include "Containers/Array.h"
#include "Core/Types.h"

class UParticleModule;
struct FBaseParticle;
struct FParticleEmitterInstance;

// 설정에서 계산하는 파생 정보입니다. 에셋에 저장하거나 실행 상태를 넣지 않습니다.
struct FParticleModuleBinding
{
	UParticleModule* Module = nullptr;
	int32 ParticleOffset = 0;
	int32 ParticleBytes = 0;
	int32 InstanceOffset = 0;
	int32 InstanceBytes = 0;
	bool bSpawn = false;
	bool bUpdate = false;
};

struct FParticleEmitterLayout
{
	TArray<FParticleModuleBinding> Modules;
	int32 ParticleSize = 0;
	int32 ParticleStride = 0;
	int32 InstancePayloadSize = 0;
};

// Payload는 정렬된 원시 저장 공간입니다. 모듈이 단순 데이터의 초기화를 담당합니다.
// 동적 자원이나 별도 소멸 처리가 필요한 객체는 Payload에 보관하지 않습니다.
struct FParticleInstanceContext
{
	FParticleEmitterInstance& Owner;
	const FParticleModuleBinding& Binding;
	void* InstancePayload;
};

struct FParticleSpawnContext
{
	FParticleEmitterInstance& Owner;
	FBaseParticle& Particle;
	const FParticleModuleBinding& Binding;
	float SpawnTime;
	void* ParticlePayload;
	void* InstancePayload;
};

struct FParticleUpdateContext
{
	FParticleEmitterInstance& Owner;
	const FParticleModuleBinding& Binding;
	float DeltaTime;
	void* InstancePayload;
};
