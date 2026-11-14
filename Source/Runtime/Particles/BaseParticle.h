#pragma once

#include "Math/Vector.h"
#include "Math/Color.h"

// 입자 하나의 현재 상태를 나타내는 구조체
struct FBaseParticle
{
	// 24
	FVector OldLocation;
	FVector Location;

	// 16
	FVector BaseVelocity;
	float Rotation;

	// 16
	FVector Velocity;
	float BaseRotationRate;

	// 16
	FVector BaseSize;
	float RotationRate;

	// 16
	FVector Size;
	int32 Flags;

	// 16
	FColor Color;
	FColor BaseColor;
	float RelativeTime;
	float Lifetime;
};