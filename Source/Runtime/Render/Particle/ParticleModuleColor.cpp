#include "EnginePCH.h"
#include "Render/Particle/ParticleModuleColor.h"
#include "Render/Particle/ParticleEmitterInstance.h"

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* OutParticle)
{
	OutParticle->Color = StartColor;
}

void UParticleModuleColor::Update(FParticleEmitterInstance* Owner, int32 Offset, float deltaTime)
{
    BEGIN_UPDATE_LOOP
    float T = Particle.RelativeTime;
    Particle.Color = FLinearColor(FMath::Lerp(StartColor.R, EndColor.R, T),
        FMath::Lerp(StartColor.G, EndColor.G, T), 
        FMath::Lerp(StartColor.B, EndColor.B, T),
        FMath::Lerp(StartColor.A, EndColor.A, T));
    END_UPDATE_LOOP
}
