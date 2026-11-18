#include "EnginePCH.h"
#include "Render/Particle/ParticleSystem.h"
#include "Curves/CurveFloat.h"

#include "Render/Particle/ParticleModuleLifetime.h"
#include "UObject/Property.h"
#include "UObject/UObjectGlobals.h"
#include "Render/Particle/ParticleModuleRequired.h"
#include "Render/Particle/ParticleModuleSpawn.h"
#include "Render/Particle/ParticleModuleLocation.h"
#include "Render/Particle/ParticleModuleVelocity.h"
#include "Render/Particle/ParticleModuleColor.h"
#include "Render/Particle/ParticleModuleColorOverLife.h"
#include "Render/Particle/ParticleModuleCollision.h"
#include "Render/Particle/ParticleModuleSize.h"
#include "Asset/AssetManager.h"
#include <cmath>

namespace
{
	int32 GetFloatComponentCount(EPropertyType Type)
	{
		switch (Type)
		{
		case EPropertyType::Float:       return 1;
		case EPropertyType::Vector:      return 3;
		case EPropertyType::LinearColor: return 4;
		default:                        return 0;
		}
	}

	bool ValidateModuleValues(const UParticleModule* Module, FString& OutError)
	{
		// 현재 모듈의 float, FVector, FLinearColor 설정에서 NaN과 무한대를 거부한다.
		for (UClass* Class = Module->GetClass(); Class; Class = Class->Super)
		{
			for (const FProperty& Property : Class->GetProperties())
			{
				const int32 Count = GetFloatComponentCount(Property.Type);
				const uint8* PropertyData = reinterpret_cast<const uint8*>(Module) + Property.Offset;
				const float* Values = reinterpret_cast<const float*>(PropertyData);

				for (int32 Index = 0; Index < Count; ++Index)
				{
					if (!std::isfinite(Values[Index]))
					{
						OutError = "Non-finite particle property: " + Property.Name;
						return false;
					}
				}
			}
		}

		return true;
	}

	bool ValidateModule(const UParticleModule* Module, const UParticleLODLevel* LOD, FString& OutError)
	{
		if (!Module || Module->GetOuter() != LOD)
		{
			OutError = "Particle module must belong to its LOD";
			return false;
		}

		if (!ValidateModuleValues(Module, OutError))
		{
			return false;
		}

		const auto* Lifetime = Cast<UParticleModuleLifetime>(Module);
		if (Lifetime && (Lifetime->MinLifetime <= 0 || Lifetime->MaxLifetime < Lifetime->MinLifetime))
		{
			OutError = "Particle lifetime must be positive and ordered";
			return false;
		}

		return true;
	}

	bool ValidateLODSettings(const UParticleLODLevel* LOD, const UParticleEmitter* Emitter, FString& OutError)
	{
		// TypeData(Mesh/Beam/Ribbon)는 이제 지원하므로 거부하지 않는다. 비어 있으면 Sprite 이미터
		if (!LOD || LOD->GetOuter() != Emitter || !LOD->GetRequiredModule() || !LOD->GetSpawnModule())
		{
			OutError = "Particle LOD requires Required/Spawn modules";
			return false;
		}

		const UParticleModuleRequired* Required = LOD->GetRequiredModule();
		const UParticleModuleSpawn* Spawn = LOD->GetSpawnModule();
		const bool bValidCapacity = Required->GetMaxParticles() >= 1 && Required->GetMaxParticles() <= 65535;
		const bool bHasSavedMaterial = Required->GetMaterial() && !Required->GetMaterial()->GetPath().empty();
		const bool bValidSpawn = Spawn->GetRate() >= 0 && Spawn->GetRateScale() >= 0 &&
			Spawn->GetBurstCount() >= 0 && std::isfinite(Spawn->GetRate() * Spawn->GetRateScale());

		if (!bValidCapacity || !bHasSavedMaterial || !bValidSpawn)
		{
			OutError = "Invalid particle capacity, material reference or spawn settings";
			return false;
		}

		return true;
	}

	bool ValidateLODModules(const UParticleLODLevel* LOD, FString& OutError)
	{
		// 전용 슬롯에 보관된 두 모듈도 일반 모듈과 같은 기준으로 검사한다.
		if (!ValidateModule(LOD->GetRequiredModule(), LOD, OutError) ||
			!ValidateModule(LOD->GetSpawnModule(), LOD, OutError))
		{
			return false;
		}

		for (const UParticleModule* Module : LOD->GetModules())
		{
			if (!ValidateModule(Module, LOD, OutError))
			{
				return false;
			}
		}

		return true;
	}
}

void UParticleSystem::AddEmitter(UParticleEmitter* InEmitter)
{
	if (!InEmitter || Emitters.Find(InEmitter) >= 0)
	{
		return;
	}

	if (InEmitter->GetOuter() && InEmitter->GetOuter() != this)
	{
		return;
	}

	InEmitter->SetOuter(this);
	Emitters.Add(InEmitter);
}

void UParticleSystem::RemoveEmitter(int32 Index)
{
	if (!Emitters.IsValidIndex(Index)) return;

	UParticleEmitter* Emitter = Emitters[Index];
	Emitters.RemoveAt(Index);
	DestroyOwnedObjects(Emitter);
	delete Emitter;
}

bool UParticleSystem::RemoveModule(int32 EmitterIndex, int32 LODIndex, UParticleModule* Module)
{
	if (!Module || !Emitters.IsValidIndex(EmitterIndex) || !Emitters[EmitterIndex]) return false;

	UParticleLODLevel* LOD = Emitters[EmitterIndex]->GetLODLevel(LODIndex);
	if (!LOD || Module->GetOuter() != LOD) return false;

	const int32 ModuleIndex = LOD->Modules.Find(Module);
	if (ModuleIndex < 0) return false;

	LOD->Modules.RemoveAt(ModuleIndex);
	Emitters[EmitterIndex]->CacheEmitterModuleInfo(LODIndex);
	DestroyOwnedObjects(Module);
	delete Module;
	return true;
}

void UParticleSystem::SwapEmitters(UParticleSystem& Other)
{
	if (this == &Other) return;

	std::swap(Emitters, Other.Emitters);
	for (UParticleEmitter* Emitter : Emitters)
	{
		Emitter->SetOuter(this);
	}
	for (UParticleEmitter* Emitter : Other.Emitters)
	{
		Emitter->SetOuter(&Other);
	}
}

bool UParticleSystem::ValidateAsset(FString& OutError) const
{
	OutError.clear();

	for (const UParticleEmitter* Emitter : Emitters)
	{
		if (!Emitter || Emitter->GetOuter() != this || Emitter->GetLODLevels().IsEmpty())
		{
			OutError = "Particle emitter must belong to the system and contain an LOD";
			return false;
		}

		for (const UParticleLODLevel* LOD : Emitter->GetLODLevels())
		{
			if (!ValidateLODSettings(LOD, Emitter, OutError) || !ValidateLODModules(LOD, OutError))
			{
				return false;
			}
		}
	}

	return true;
}

void UParticleSystem::PostLoadAsset()
{
	for (UParticleEmitter* Emitter : Emitters)
		Emitter->CacheEmitterModuleInfo();
}

UParticleSystem* UParticleSystem::CreateDefault()
{
	UParticleSystem* System = NewObject<UParticleSystem>();
	System->AddDefaultEmitter();
	return System;
}

void UParticleSystem::AddDefaultEmitter()
{
	UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
	Required->SetMaxParticles(300);
	Required->SetMaterial(UAssetManager::GetAssetByPath<UMaterial>("ParticleSpriteMaterial"));

	UParticleModuleSpawn* Spawn = NewObject<UParticleModuleSpawn>();
	Spawn->SetRate(60.0f);

	UParticleModuleLifetime* Lifetime = NewObject<UParticleModuleLifetime>();
	Lifetime->MinLifetime = 1.0f;
	Lifetime->MaxLifetime = 2.0f;

	UParticleModuleLocation* Location = NewObject<UParticleModuleLocation>();
	Location->StartLocationMin = FVector(-0.2f, -0.2f, 0.0f);
	Location->StartLocationMax = FVector( 0.2f,  0.2f, 0.0f);

	UParticleModuleVelocity* Velocity = NewObject<UParticleModuleVelocity>();
	Velocity->StartVelocityMin = FVector(-0.3f, -0.3f, 1.0f);
	Velocity->StartVelocityMax = FVector( 0.3f,  0.3f, 2.0f);

	UParticleModuleColor* Color = NewObject<UParticleModuleColor>();
	Color->StartColor = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);

	UParticleModuleColorOverLife* ColorOverLife = NewObject<UParticleModuleColorOverLife>();
	ColorOverLife->StartColor = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);
	ColorOverLife->EndColor   = FLinearColor(1.0f, 0.1f, 0.0f, 0.0f);   // 붉어지며 사라진다

	UCurveFloat* ColorBlendCurve = NewObject<UCurveFloat>(ColorOverLife);
	ColorBlendCurve->AddKey(0.0f, 0.0f);
	ColorBlendCurve->AddKey(0.75f, 0.0f);
	ColorBlendCurve->AddKey(1.0f, 1.0f);
	ColorOverLife->ColorBlendCurve = ColorBlendCurve;

	UParticleModuleSize* Size = NewObject<UParticleModuleSize>();
	Size->StartSizeMin = FVector(0.2f, 0.2f, 0.2f);
	Size->StartSizeMax = FVector(0.4f, 0.4f, 0.4f);

	UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
	LODLevel->SetRequiredModule(Required);
	LODLevel->SetSpawnModule(Spawn);
	LODLevel->AddModule(Lifetime);
	LODLevel->AddModule(Location);
	LODLevel->AddModule(Velocity);
	LODLevel->AddModule(Color);
	LODLevel->AddModule(ColorOverLife);
	LODLevel->AddModule(Size);
	LODLevel->AddModule(NewObject<UParticleModuleCollision>());

	UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
	Emitter->AddLODLevel(LODLevel);
	UParticleLODLevel* FarLOD = Cast<UParticleLODLevel>(StaticDuplicateObject(LODLevel, Emitter, FName("LOD1")));
	if (FarLOD)
	{
		FarLOD->GetSpawnModule()->SetRate(5.0f);
		Emitter->AddLODLevel(FarLOD);
	}
	Emitter->CacheEmitterModuleInfo();   // 모듈을 다 넣은 뒤 Spawn/Update 목록 분류

	AddEmitter(Emitter);
}
