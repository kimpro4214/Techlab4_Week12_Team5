#include "EnginePCH.h"
#include "ParticleSystemActor.h"
#include "Render/Particle/ParticleSystem.h"

AParticleSystemActor::AParticleSystemActor()
{
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");
	ParticleSystemComponent->SetTemplate(UParticleSystem::CreateDefault());
	ParticleSystemComponent->SetupAttachment(GetRootComponent());
}
