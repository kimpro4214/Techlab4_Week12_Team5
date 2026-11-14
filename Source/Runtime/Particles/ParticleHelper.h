#pragma once

// Source: /UnrealEngine/Engine/Source/Runtime/Engine/Public/ParticleHelper.h

#define	PARTICLE_PREFETCH(Index)					FPlatformMisc::Prefetch( ParticleData, ParticleStride * ParticleIndices[Index] )
#define PARTICLE_INSTANCE_PREFETCH(Instance, Index)	FPlatformMisc::Prefetch( Instance->ParticleData, Instance->ParticleStride * Instance->ParticleIndices[Index] )
#define	PARTICLE_OWNER_PREFETCH(Index)				FPlatformMisc::Prefetch( Owner->ParticleData, Owner->ParticleStride * Owner->ParticleIndices[Index] )

#define DECLARE_PARTICLE(Name,Address)		\
	FBaseParticle& Name = *((FBaseParticle*) (Address));

#define DECLARE_PARTICLE_CONST(Name,Address)		\
	const FBaseParticle& Name = *((const FBaseParticle*) (Address));

#define DECLARE_PARTICLE_PTR(Name, Address)				\
	FBaseParticle* Name = (FBaseParticle*) (Address);	\

#define BEGIN_UPDATE_LOOP																								\
	{																													\
		int32&			ActiveParticles = Context.Owner.ActiveParticles;												\
		int32			Offset			= Context.Offset;																\
		uint32			CurrentOffset	= Offset;																		\
		float			DeltaTime		= Context.DeltaTime;															\
		const uint8*	ParticleData	= Context.Owner.ParticleData;													\
		const uint32	ParticleStride	= Context.Owner.ParticleStride;													\
		uint16*			ParticleIndices	= Context.Owner.ParticleIndices;												\
		for(int32 i=ActiveParticles-1; i>=0; i--)																		\
		{																												\
			const int32	CurrentIndex	= ParticleIndices[i];															\
			const uint8* ParticleBase	= ParticleData + CurrentIndex * ParticleStride;									\
			FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);												\
			if ((Particle.Flags & STATE_Particle_Freeze) == 0)															\
			{																											\

#define END_UPDATE_LOOP																									\
			}																											\
			CurrentOffset				= Offset;																		\
		}																												\
	}

#define CONTINUE_UPDATE_LOOP																							\
		CurrentOffset = Offset;																							\
		continue;

#define SPAWN_INIT																										\
	const int32		ActiveParticles	= Context.Owner.ActiveParticles;													\
	const uint32	ParticleStride	= Context.Owner.ParticleStride;														\
	uint32			CurrentOffset	= Context.Offset;																	\
	FBaseParticle*	ParticleBase	= Context.ParticleBase;																\
	FBaseParticle&	Particle		= *(ParticleBase);

#define PARTICLE_ELEMENT(Type,Name)																						\
	Type& Name = *((Type*)((uint8*)ParticleBase + CurrentOffset));														\
	CurrentOffset += sizeof(Type);

#define KILL_CURRENT_PARTICLE																							\
	{																													\
		ParticleIndices[i]					= ParticleIndices[ActiveParticles-1];										\
		ParticleIndices[ActiveParticles-1]	= CurrentIndex;																\
		ActiveParticles--;																								\
	}