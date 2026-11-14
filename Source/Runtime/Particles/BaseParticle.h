#pragma once

#include "Math/Vector.h"
#include "Math/Color.h"

// 입자 하나의 현재 상태를 나타내는 구조체
struct FBaseParticle
{
	FVector Location;
	FVector Velocity;
	FVector BaseVelocity;
	float RelativeTime;
	float Lifetime;
	float Rotation;
	float RotationRate;
	FVector Size;
	FColor Color;
};