#include "EnginePCH.h"
#include "Render/Particle/SpriteBatcher.h"
#include "Render/RenderCommand.h"

bool FSpriteBatcher::Init(uint32 InMaxSprites)
{
	// TODO: Dynamic VB 생성 (RenderCommand::CreateDynamicVertexBuffer)
	// TODO: 인덱스 패턴을 채운 정적 IB 생성 (RenderCommand::CreateStaticIndexBuffer)
	return false;
}

void FSpriteBatcher::Begin()
{
	// TODO
}

void FSpriteBatcher::AddSprite(
	const FVector& WorldCenter,
	const FVector2& Size,
	const FVector4& Color,
	const FVector& CameraRight,
	const FVector& CameraUp,
	const FVector2& UVMin,
	const FVector2& UVMax)
{
	// TODO
}

void FSpriteBatcher::End()
{
	// TODO: RenderCommand::MapWriteDiscard / Unmap
}

FMeshGeometry FSpriteBatcher::GetGeometry() const
{
	FMeshGeometry Geometry;
	Geometry.VertexBuffer = VertexBuffer.get();
	Geometry.IndexBuffer = IndexBuffer.get();
	return Geometry;
}
