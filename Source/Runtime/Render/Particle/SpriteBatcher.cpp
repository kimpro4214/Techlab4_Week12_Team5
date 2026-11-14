#include "EnginePCH.h"
#include "Render/Particle/SpriteBatcher.h"
#include "Render/RenderCommand.h"

bool FSpriteBatcher::Init(uint32 InMaxSprites)
{
	if (InMaxSprites == 0) return false;
	MaxSprites = InMaxSprites;

	const uint32 VertexByte = MaxSprites * 4 * sizeof(FParticleSystemVertex);
	VertexBuffer = RenderCommand::CreateDynamicVertexBuffer(VertexByte, sizeof(FParticleSystemVertex));
	// MakeUnique는 항상 객체를 만들고, D3D 생성 실패 시 내부 버퍼만 null로 남는다
	if (!VertexBuffer || !VertexBuffer->GetBuffer()) return false;

	const uint32 BaseIndices[6] = { 0,1,2,0,2,3 }; // 사각형 Quad 기준 정해진 Offset
	TArray<uint32> Indices;
	Indices.SetNum(MaxSprites * 6);
	for (uint32 i = 0; i < MaxSprites; i++)
	{
		const uint32 Base = i * 4;	// Sprite 하나당 Quad이므로 사각형 정점 4개 Stride
		for (uint32 j = 0; j < 6; j++)
		{
			Indices[i * 6 + j] = Base + BaseIndices[j];	// Quad 정해진 기준으로 Index 더하기
		}
	}
	IndexBuffer = RenderCommand::CreateStaticIndexBuffer(Indices.GetData(), MaxSprites * 6);
	if (!IndexBuffer || !IndexBuffer->GetBuffer()) return false;

	// AddSprite 중 재할당을 막기 위해 최대 정점 수만큼 미리 확보
	Vertices.Reserve(MaxSprites * 4);
	return true;
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
