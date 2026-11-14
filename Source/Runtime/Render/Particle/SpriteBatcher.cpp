#include "EnginePCH.h"
#include "Render/Particle/SpriteBatcher.h"
#include "Render/RenderCommand.h"

bool FSpriteBatcher::Init(uint32 InMaxSprites)
{
	if (InMaxSprites == 0) return false;
	MaxSprites = InMaxSprites;

	const uint32 VertexByte = MaxSprites * 4 * sizeof(FParticleSystemVertex);
	VertexBuffer = RenderCommand::CreateDynamicVertexBuffer(VertexByte, sizeof(FParticleSystemVertex));
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
	Vertices.Reset();
	SpriteCount = 0;
}

void FSpriteBatcher::AddSprite(				// Quad Sprite Vertex 생성 후 Add
	const FVector& WorldCenter,
	const FVector2& Size,
	const FVector4& Color,
	const FVector& CameraRight,
	const FVector& CameraUp,
	const FVector2& UVMin,
	const FVector2& UVMax)
{
	if (SpriteCount >= MaxSprites) return;
	FVector WidthVec = CameraRight * (Size.X * 0.5f);
	FVector HeightVec = CameraUp * (Size.Y * 0.5f);
	FParticleSystemVertex Vertex0;
	Vertex0.Position = WorldCenter - WidthVec - HeightVec;
	Vertex0.UV = FVector2(UVMin.X, UVMax.Y);
	Vertex0.Color = Color;
	FParticleSystemVertex Vertex1;
	Vertex1.Position = WorldCenter - WidthVec + HeightVec;
	Vertex1.UV = FVector2(UVMin.X, UVMin.Y);
	Vertex1.Color = Color;
	FParticleSystemVertex Vertex2;
	Vertex2.Position = WorldCenter + WidthVec + HeightVec;
	Vertex2.UV = FVector2(UVMax.X, UVMin.Y);
	Vertex2.Color = Color;
	FParticleSystemVertex Vertex3;
	Vertex3.Position = WorldCenter + WidthVec - HeightVec;
	Vertex3.UV = FVector2(UVMax.X, UVMax.Y);
	Vertex3.Color = Color;

	Vertices.Add(Vertex0);
	Vertices.Add(Vertex1);
	Vertices.Add(Vertex2);
	Vertices.Add(Vertex3);

	SpriteCount++;
}

void FSpriteBatcher::End()
{
	if (SpriteCount == 0) return;
	void* pData = RenderCommand::MapWriteDiscard(VertexBuffer.get());
	if (pData == nullptr)
	{
		SpriteCount = 0;
		return;
	}
	memcpy(pData, Vertices.GetData(), SpriteCount * 4 * sizeof(FParticleSystemVertex));
	RenderCommand::Unmap(VertexBuffer.get());
}

FMeshGeometry FSpriteBatcher::GetGeometry() const
{
	FMeshGeometry Geometry;
	Geometry.VertexBuffer = VertexBuffer.get();
	Geometry.IndexBuffer = IndexBuffer.get();
	return Geometry;
}
