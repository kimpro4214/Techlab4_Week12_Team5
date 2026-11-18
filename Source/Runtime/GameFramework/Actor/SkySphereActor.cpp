#include "EnginePCH.h"
#include "SkySphereActor.h"
#include "Components/SkySphereComponent.h"

ASkySphereActor::ASkySphereActor()
{
	SetRootComponent(CreateDefaultSubobject<USkySphereComponent>("SkySphereComponent"));
}
