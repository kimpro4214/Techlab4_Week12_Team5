#include "EnginePCH.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"

UParticleLODLevel::~UParticleLODLevel()
{
	for (UParticleModule* Module : Modules)
	{
		delete Module;
	}
}

void UParticleLODLevel::InitializeDefaultModules()
{
	// Required, Spawn 모듈 생성

	if (!RequiredModule)
	{
		AddModule(UParticleModuleRequired::StaticClass());
	}

	if (!SpawnModule)
	{
		AddModule(UParticleModuleSpawn::StaticClass());
	}
}

UParticleModule* UParticleLODLevel::AddModule(UClass* ModuleClass)
{
	// ModuleClass가 UParticleModule의 자식인지 확인
	if (!ModuleClass ||
		!ModuleClass->IsChildOf(UParticleModule::StaticClass()) ||
		!ModuleClass->Constructor // 순수 가상 함수가 있는 추상 클래스도 거부
	)
	{
		return nullptr;
	}

	// 특수 모듈 여부를 bool로 저장
	const bool bIsRequired = ModuleClass->IsChildOf(UParticleModuleRequired::StaticClass());
	const bool bIsSpawn = ModuleClass->IsChildOf(UParticleModuleSpawn::StaticClass());
	const bool bIsTypeData = ModuleClass->IsChildOf(UParticleModuleTypeDataBase::StaticClass());

	// 특수 모듈은 LOD별로 하나만 허용
	if ((bIsRequired && RequiredModule) ||
		(bIsSpawn && SpawnModule) ||
		(bIsTypeData && TypeDataModule)
	)
	{
		return nullptr;
	}

	UParticleModule* Module = NewObject<UParticleModule>(this, ModuleClass);

	Modules.Add(Module);

	if (bIsRequired)
	{
		RequiredModule = static_cast<UParticleModuleRequired*>(Module);
	}
	else if (bIsSpawn)
	{
		SpawnModule = static_cast<UParticleModuleSpawn*>(Module);
	}
	else if (bIsTypeData)
	{
		TypeDataModule = static_cast<UParticleModuleTypeDataBase*>(Module);
	}

	// 구성이 변경되었으므로 이전 실행 목록은 사용하지 않습니다.
	ClearModuleLists();

	return Module;
}

bool UParticleLODLevel::RemoveModule(UParticleModule* Module)
{
	// 모듈이 없거나, 기본 모듈이면 스킵
	if (!Module ||
		Module == RequiredModule ||
		Module == SpawnModule
	)
	{
		return false;
	}

	for (int32 Index = 0; Index < Modules.Num(); ++Index)
	{
		if (Modules[Index] != Module)
		{
			continue;
		}

		// 모듈을 찾음

		ClearModuleLists();

		if (Module == TypeDataModule)
		{
			TypeDataModule = nullptr;
		}

		// swap 방식 없이 해당 index 제거 후 원소를 당겨옴
		Modules.RemoveAt(Index);
		delete Module;

		return true;
	}

	return false;
}

bool UParticleLODLevel::Validate() const
{
	// 레벨이 올바른지, 기본 모듈이 있는지
	if (Level < 0 || !RequiredModule || !SpawnModule)
	{
		return false;
	}

	for (const UParticleModule* Module : Modules)
	{
		// nullptr, 소유자 체크
		if (!Module || Module->GetOuter() != this)
		{
			return false;
		}

		// 활성화된 모듈의 Validate가 성공하는지
		if (Module->IsEnabled() && !Module->Validate())
		{
			return false;
		}
	}

	return true;
}

bool UParticleLODLevel::BuildModuleLists()
{
	// 기존 목록 비우기
	ClearModuleLists();

	if (!Validate())
	{
		return false;
	}

	// 아직 활성화되지 않았다면, 빈 목록으로 유지
	if (!bEnabled)
	{
		return true;
	}

	for (UParticleModule* Module : Modules)
	{
		if (!Module->IsEnabled())
		{
			continue;
		}

		// 기본 모듈들은 제외
		if (Module == RequiredModule ||
			Module == SpawnModule ||
			Module == TypeDataModule
		)
		{
			continue;
		}

		if (Module->IsSpawnModule())
		{
			SpawnModules.Add(Module);
		}

		if (Module->IsUpdateModule())
		{
			UpdateModules.Add(Module);
		}
	}

	return true;
}

void UParticleLODLevel::ClearModuleLists()
{
	SpawnModules.Reset();
	UpdateModules.Reset();
}