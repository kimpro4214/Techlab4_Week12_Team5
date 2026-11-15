#include "EnginePCH.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"

UParticleSystem::~UParticleSystem()
{
	// 소유하는 Emitter도 같이 해제
	for (UParticleEmitter* Emitter : Emitters)
	{
		delete Emitter;
	}
}

bool UParticleSystem::Validate() const
{
	// 내부 Emitter들이 잘 되어있는지 검증

	if (Emitters.IsEmpty())
	{
		return false;
	}

	for (const UParticleEmitter* Emitter : Emitters)
	{
		if (!Emitter ||
			Emitter->GetOuter() != this ||
			!Emitter->Validate())
		{
			return false;
		}
	}

	return true;
}

bool UParticleSystem::Build()
{
	if (!Validate())
	{
		return false;
	}

	for (UParticleEmitter* Emitter : Emitters)
	{
		if (!Emitter->Build())
		{
			return false;
		}
	}

	return true;
}

UParticleEmitter* UParticleSystem::AddEmitter()
{
	// 새로운 Emitter 추가
	UParticleEmitter* NewEmitter = NewObject<UParticleEmitter>(this);

	Emitters.Add(NewEmitter);

	return NewEmitter;
}

UParticleEmitter* UParticleSystem::GetEmitter(int32 Index) const
{
	if (!Emitters.IsValidIndex(Index))
	{
		return nullptr;
	}

	return Emitters[Index];
}