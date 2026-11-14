#pragma once

#include "EnginePCH.h"
#include "Render/Buffer.h"
#include "Render/MeshGeometry.h"
#include "Render/Vertex.h"

// Sprite Emitter 하나의 파티클들을 사각형(정점 4개)으로 펼쳐 Dynamic VB 하나에 모으는 도구.
// Emitter Instance마다 하나씩 PSC가 소유한다. (Instance는 Batcher를 모른다)
// 사용 순서: Init(한 번) → [매 뷰] Begin → AddSprite 반복 → End → GetGeometry로 패킷에 연결
class FSpriteBatcher
{
public:
	FSpriteBatcher() = default;
	~FSpriteBatcher() = default;

	// 최대 스프라이트 수만큼 Dynamic VB와 정적 IB(0,1,2,0,2,3 패턴)를 만든다
	bool Init(uint32 InMaxSprites);

	// 이번 뷰에 쓸 CPU 임시 정점 배열과 개수를 초기화한다
	void Begin();

	// 월드 좌표 중심을 카메라 Right/Up 방향으로 펼쳐 정점 4개를 추가한다
	// UV 범위는 아틀라스 셀 영역. 아틀라스를 안 쓰면 (0,0)~(1,1)
	void AddSprite(
		const FVector& WorldCenter,
		const FVector2& Size,
		const FVector4& Color,
		const FVector& CameraRight,
		const FVector& CameraUp,
		const FVector2& UVMin = FVector2(0.0f, 0.0f),
		const FVector2& UVMax = FVector2(1.0f, 1.0f));

	// 모은 정점을 Dynamic VB에 업로드한다 (Map WRITE_DISCARD → 복사 → Unmap)
	void End();

	// 렌더 패킷에 넣을 VB/IB. 버퍼 소유권은 Batcher에 있다
	FMeshGeometry GetGeometry() const;

	uint32 GetSpriteCount() const { return SpriteCount; }
	uint32 GetIndexCount() const { return SpriteCount * 6; }

private:
	TArray<FParticleSystemVertex> Vertices;		// CPU 임시 정점 배열 (End에서 VB로 복사)
	TUniquePtr<FVertexBuffer> VertexBuffer;		// Dynamic VB: MaxSprites × 4 정점
	TUniquePtr<FIndexBuffer> IndexBuffer;		// 정적 IB: MaxSprites × 6 인덱스

	uint32 MaxSprites = 0;		// 버퍼 용량 (넘치면 버림)
	uint32 SpriteCount = 0;		// 이번 뷰에 추가된 스프라이트 수
};
