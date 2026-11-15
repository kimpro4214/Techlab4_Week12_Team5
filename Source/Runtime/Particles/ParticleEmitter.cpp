#include "EnginePCH.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "BaseParticle.h"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace
{
	bool AppendPayload(int32 Bytes, int32 Alignment, uint64& Cursor, int32& Offset)
	{
		if (Bytes < 0 ||
			Alignment <= 0 ||
			(Alignment & (Alignment - 1)) != 0 ||
			Alignment > alignof(std::max_align_t))
		{
			return false;
		}

		if (Bytes == 0)
		{
			Offset = 0;
			return true;
		}

		const uint64 Aligned = (Cursor + Alignment - 1) & ~static_cast<uint64>(Alignment - 1);
		const uint64 End = Aligned + static_cast<uint64>(Bytes);
		if (End > static_cast<uint64>(std::numeric_limits<int32>::max()))
		{
			return false;
		}

		Offset = static_cast<int32>(Aligned);
		Cursor = End;
		return true;
	}
}

UParticleEmitter::~UParticleEmitter()
{
	// 소유하는 LOD Level도 같이 해제
	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		delete LODLevel;
	}
}

bool UParticleEmitter::Validate() const
{
	// 내부 LOD Level이 잘 되어있는지 검증

	if (LODLevels.IsEmpty())
	{
		return false;
	}

	for (int32 Index = 0; Index < LODLevels.Num(); ++Index)
	{
		const UParticleLODLevel* LODLevel = LODLevels[Index];

		if (!LODLevel ||
			LODLevel->GetOuter() != this ||
			LODLevel->GetLevel() != Index ||
			!LODLevel->Validate())
		{
			return false;
		}
	}

	return true;
}

bool UParticleEmitter::Build()
{
	ModuleLayouts.Reset();

	if (!Validate())
	{
		return false;
	}

	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (!LODLevel->BuildModuleLists())
		{
			return false;
		}
	}

	return CacheEmitterModuleInfo();
}


UParticleLODLevel* UParticleEmitter::AddLODLevel()
{
	// 새로운 LOD 레벨 추가
	UParticleLODLevel* NewLevel = NewObject<UParticleLODLevel>(this);

	// 기존 레벨 +1로 부여
	NewLevel->Level = LODLevels.Num();
	LODLevels.Add(NewLevel);

	// 기본 모듈을 초기화
	NewLevel->InitializeDefaultModules();

	return NewLevel;
}

UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 Index) const
{
	if (!LODLevels.IsValidIndex(Index))
	{
		return nullptr;
	}

	return LODLevels[Index];
}

bool UParticleEmitter::CacheEmitterModuleInfo()
{
	ModuleLayouts.Reset();

	if (!Validate())
	{
		return false;
	}

	TArray<FParticleEmitterLayout> NewLayouts;

	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		FParticleEmitterLayout Layout;
		uint64 ParticleCursor = sizeof(FBaseParticle);
		uint64 InstanceCursor = 0;
		int32 ParticleAlignment = alignof(FBaseParticle);

		for (UParticleModule* Module : LODLevel->GetModules())
		{
			if (!LODLevel->IsEnabled() || !Module->IsEnabled())
			{
				continue;
			}

			FParticleModuleBinding Binding;
			Binding.Module = Module;
			Binding.ParticleBytes = Module->RequiredBytes();
			Binding.InstanceBytes = Module->RequiredBytesPerInstance();

			if (!AppendPayload(Binding.ParticleBytes, Module->GetParticlePayloadAlignment(),
				ParticleCursor, Binding.ParticleOffset) ||
				!AppendPayload(Binding.InstanceBytes, Module->GetInstancePayloadAlignment(),
				InstanceCursor, Binding.InstanceOffset))
			{
				return false;
			}

			if (Binding.ParticleBytes > 0)
			{
				ParticleAlignment = std::max(ParticleAlignment, Module->GetParticlePayloadAlignment());
			}

			const bool bSpecial = Module == LODLevel->GetRequiredModule() ||
				Module == LODLevel->GetSpawnModule() ||
				Module == LODLevel->GetTypeDataModule();
			Binding.bSpawn = !bSpecial && Module->IsSpawnModule();
			Binding.bUpdate = !bSpecial && Module->IsUpdateModule();
			Layout.Modules.Add(Binding);
		}

		Layout.ParticleSize = static_cast<int32>(ParticleCursor);
		const uint64 Stride = (ParticleCursor + ParticleAlignment - 1)
			& ~static_cast<uint64>(ParticleAlignment - 1);
		// 인스턴스 영역도 기본 정렬 단위로 올려 컨테이너의 정렬 검사를 만족합니다.
		const uint64 InstanceSize = (InstanceCursor + alignof(std::max_align_t) - 1)
			& ~static_cast<uint64>(alignof(std::max_align_t) - 1);
		if (Stride > static_cast<uint64>(std::numeric_limits<int32>::max()) ||
			InstanceSize > static_cast<uint64>(std::numeric_limits<int32>::max()))
		{
			return false;
		}

		Layout.ParticleStride = static_cast<int32>(Stride);
		Layout.InstancePayloadSize = static_cast<int32>(InstanceSize);
		NewLayouts.Add(std::move(Layout));
	}

	ModuleLayouts = std::move(NewLayouts);
	return true;
}

const FParticleEmitterLayout* UParticleEmitter::GetModuleLayout(int32 LODIndex) const
{
	return ModuleLayouts.IsValidIndex(LODIndex) ? &ModuleLayouts[LODIndex] : nullptr;
}
