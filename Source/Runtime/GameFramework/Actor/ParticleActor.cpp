#include "EnginePCH.h"
#include "ParticleActor.h"

AParticleActor::AParticleActor()
{
	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>("UParticleSystemComponent");
	ParticleComponent->SetupAttachment(GetRootComponent());
}
