#include "EnginePCH.h"
#include "SkySphereComponent.h"
#include "Engine/World.h"

void USkySphereComponent::OnRegister()
{
	Super::OnRegister();
	GetWorld()->GetScene().AddSkySphere(this);
}

void USkySphereComponent::OnUnregister()
{
	GetWorld()->GetScene().RemoveSkySphere(this);
	Super::OnUnregister();
}
