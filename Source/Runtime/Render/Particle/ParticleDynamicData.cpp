#include "EnginePCH.h"
#include "Render/Particle/ParticleDynamicData.h"
#include "Render/RenderCommand.h"
#include "Render/Material.h"
#include <algorithm>
#include <cmath>

void FDynamicSpriteEmitterData::Init(int32 MaxParticles)
{
	// 이번 프레임의 뷰 버퍼를 처음부터 다시 쓴다
	NextViewIndex = 0;

	if (MaxParticles <= Capacity)
		return;

	// 용량이 늘었으면 버퍼를 새 크기로 다시 만든다. VB는 AcquireViewResources에서 필요할 때 만든다
	Capacity = MaxParticles;
	ViewResources.Reset();
	BuildIndexBuffer(Capacity);
}

void FDynamicSpriteEmitterData::BuildIndexBuffer(int32 MaxParticles)
{
	// 파티클당 정점 4개 → 최대 65535 × 4 = 262140개라 uint16을 넘는다. 반드시 uint32
	TArray<uint32> Indices;
	Indices.SetNum(MaxParticles * 6);
	for (int32 i = 0; i < MaxParticles; ++i)
	{
		const uint32 Base = static_cast<uint32>(i) * 4;
		Indices[i * 6 + 0] = Base + 0;
		Indices[i * 6 + 1] = Base + 1;
		Indices[i * 6 + 2] = Base + 2;
		Indices[i * 6 + 3] = Base + 0;
		Indices[i * 6 + 4] = Base + 2;
		Indices[i * 6 + 5] = Base + 3;
	}
	IndexBuffer = RenderCommand::CreateStaticIndexBuffer(Indices.GetData(), static_cast<uint32>(MaxParticles * 6));
}

FDynamicSpriteEmitterData::FViewResources* FDynamicSpriteEmitterData::AcquireViewResources()
{
	if (NextViewIndex >= MaxViewsPerFrame)
		return nullptr;

	// 처음 보는 뷰 번호면 그 뷰 전용 VB를 만든다. 이후 프레임부터는 재사용
	if (NextViewIndex >= ViewResources.Num())
	{
		TUniquePtr<FViewResources> NewResources = MakeUnique<FViewResources>();
		NewResources->VertexBuffer = RenderCommand::CreateDynamicVertexBuffer(
			static_cast<uint32>(Capacity * 4 * sizeof(FParticleSpriteVertex)),
			static_cast<uint32>(sizeof(FParticleSpriteVertex)));
		ViewResources.Add(std::move(NewResources));
	}

	FViewResources* Resources = ViewResources[NextViewIndex++].get();
	return Resources->VertexBuffer ? Resources : nullptr;
}

void FDynamicSpriteEmitterData::BuildDrawOrder(int32 Count, const FViewContext& View)
{
	DrawOrder.SetNum(Count, false);

	if (Source.SortMode == EParticleSortMode::None)
	{
		for (int32 i = 0; i < Count; ++i)
			DrawOrder[i] = { 0.0f, i };
		return;
	}

	// 파티클 위치는 시뮬레이션 공간이라 월드로 옮긴 뒤 카메라와 비교한다
	const bool bViewDepth = Source.SortMode == EParticleSortMode::ViewDepth;
	for (int32 i = 0; i < Count; ++i)
	{
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			Source.ParticleData + Source.ParticleIndices[i] * Source.ParticleStride);

		const FVector Delta = Source.SimulationToWorld.TransformPosition(Particle.Location) - View.CameraPosition;
		DrawOrder[i].Depth = bViewDepth ? Delta.Dot(View.CameraForward) : Delta.Dot(Delta);
		DrawOrder[i].Slot  = i;
	}

	// 먼 것부터 그린다 (Back-to-Front)
	std::sort(DrawOrder.begin(), DrawOrder.end(),
		[](const FParticleOrder& A, const FParticleOrder& B) { return A.Depth > B.Depth; });
}

void FDynamicSpriteEmitterData::Submit(FRenderQueue& RenderQueue, const FViewContext& View)
{
	const int32 Count = std::min(Source.ActiveParticleCount, Capacity);
	if (Count <= 0 || !Source.Material || !IndexBuffer)
		return;

	// 뷰마다 별도 VB. 여러 뷰를 제출한 뒤 한꺼번에 그리므로 하나를 덮어쓰면 앞 뷰가 깨진다
	FViewResources* Resources = AcquireViewResources();
	if (!Resources)
		return;

	// 1) 이 뷰에서 그릴 순서. 카메라가 뷰마다 다르므로 뷰마다 정렬한다
	BuildDrawOrder(Count, View);

	// 2) 파티클 하나당 정점 4개로 펼친다. 실제 코너 위치 계산은 VS가 카메라 축으로 한다
	//    순서는 인덱스 패턴(0,1,2 / 0,2,3)과 맞물린다: 좌하 → 좌상 → 우상 → 우하
	static const FVector2 Corners[4] = {
		FVector2(-0.5f, -0.5f), FVector2(-0.5f, 0.5f), FVector2(0.5f, 0.5f), FVector2(0.5f, -0.5f) };

	ScratchVertices.SetNum(Count * 4, false);
	for (int32 i = 0; i < Count; ++i)
	{
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			Source.ParticleData + Source.ParticleIndices[DrawOrder[i].Slot] * Source.ParticleStride);

		// 음수 스케일(미러링)이어도 사각형이 뒤집히지 않게 절댓값
		const FVector2 Size(std::abs(Particle.Size.X * Source.Scale.X), std::abs(Particle.Size.Y * Source.Scale.Y));

		for (int32 c = 0; c < 4; ++c)
		{
			FParticleSpriteVertex& Vertex = ScratchVertices[i * 4 + c];
			Vertex.Position     = Particle.Location;
			Vertex.RelativeTime = Particle.RelativeTime;
			Vertex.Size         = Size;
			Vertex.Corner       = Corners[c];
			Vertex.Rotation     = Particle.Rotation;
			Vertex.Color        = FVector4(Particle.Color.R, Particle.Color.G, Particle.Color.B, Particle.Color.A);
		}
	}

	// 3) 이 뷰 전용 VB에 올린다 (Map WRITE_DISCARD)
	RenderCommand::UpdateBufferData(Resources->VertexBuffer.get(), ScratchVertices.GetData(),
		static_cast<uint32>(Count * 4 * sizeof(FParticleSpriteVertex)));

	// 4) 카메라 축과 블렌드 방식 상수 (b1)
	View.GetBillboardAxes(Resources->Constants.CameraRight, Resources->Constants.CameraUp);
	Resources->Constants.SoftCircle       = 0.0f;
	Resources->Constants.PremultiplyAlpha = Source.Material->BlendState == EBlendState::Additive ? 1.0f : 0.0f;

	// 5) 이미터 하나 = 패킷 하나 = DrawIndexed 한 번
	const FVector Delta = Source.EmitterWorldOrigin - View.CameraPosition;

	FRenderPacket Packet;
	Packet.Model                    = RenderQueue.StoreWorldMatrix(Source.SimulationToWorld);   // b2 World
	Packet.Geometry.VertexBuffer    = Resources->VertexBuffer.get();
	Packet.Geometry.IndexBuffer     = IndexBuffer.get();
	Packet.IndexCount               = static_cast<uint32>(Count * 6);
	Packet.Material                 = Source.Material;
	Packet.MaterialParamData        = &Resources->Constants;
	Packet.MaterialParamDataSize    = sizeof(FParticleSpriteConstants);
	Packet.CameraToParticleDistance = Delta.Dot(Delta);   // 이미터끼리의 뒤→앞 순서
	RenderQueue.Add(Packet);
}

FMatrix FDynamicMeshEmitterData::BuildParticleMatrix(const FBaseParticle& Particle) const
{
	// 행 벡터 규칙(v × M): 0~2행이 메시 X/Y/Z축(크기 포함), 3행이 위치
	const FVector Up(0.0f, 0.0f, 1.0f);
	FVector AxisX(1.0f, 0.0f, 0.0f);
	FVector AxisY(0.0f, 1.0f, 0.0f);
	FVector AxisZ = Up;

	bool bAligned = false;
	if (Source.Alignment == EParticleMeshAlignment::Velocity && Particle.Velocity.SizeSquared() > 1e-8f)
	{
		// X축을 진행 방향으로. 수직으로 날아가면 Up과 평행해서 외적이 0이 되므로 다른 기준축을 쓴다
		AxisX = Particle.Velocity.Normalized();
		FVector Side = FVector::Cross(Up, AxisX);
		if (Side.SizeSquared() < 1e-6f)
			Side = FVector::Cross(FVector(1.0f, 0.0f, 0.0f), AxisX);
		AxisY = Side.Normalized();
		AxisZ = FVector::Cross(AxisX, AxisY);
		bAligned = true;
	}

	if (!bAligned)
	{
		// 진행 방향이 없으면 파티클 Rotation만큼 Z축으로 돌린다
		const float S = std::sin(Particle.Rotation);
		const float C = std::cos(Particle.Rotation);
		AxisX = FVector( C, S, 0.0f);
		AxisY = FVector(-S, C, 0.0f);
	}

	const FVector Size(Particle.Size.X * Source.Scale.X, Particle.Size.Y * Source.Scale.Y, Particle.Size.Z * Source.Scale.Z);
	const FVector X = AxisX * Size.X;
	const FVector Y = AxisY * Size.Y;
	const FVector Z = AxisZ * Size.Z;
	const FVector& P = Particle.Location;

	return FMatrix(
		FVector4(X.X, X.Y, X.Z, 0.0f),
		FVector4(Y.X, Y.Y, Y.Z, 0.0f),
		FVector4(Z.X, Z.Y, Z.Z, 0.0f),
		FVector4(P.X, P.Y, P.Z, 1.0f));
}

bool FDynamicMeshEmitterData::UsesInstancing() const
{
	return Source.Material && Source.Material->PSOType == EPSOType::ParticleMesh;
}

void FDynamicMeshEmitterData::Init()
{
	UploadedCount = 0;
	if (!UsesInstancing() || !Source.Mesh)
		return;

	const int32 Count = std::min(Source.ActiveParticleCount, Source.MaxParticles);
	if (Count <= 0)
		return;

	// 용량이 늘었을 때만 다시 만든다
	if (!InstanceBuffer || Source.MaxParticles > InstanceCapacity)
	{
		InstanceCapacity = Source.MaxParticles;
		InstanceBuffer = RenderCommand::CreateDynamicStructuredBuffer(
			static_cast<uint32>(InstanceCapacity), static_cast<uint32>(sizeof(FParticleMeshInstance)));
		if (!InstanceBuffer)
			return;
	}

	ScratchInstances.SetNum(Count, false);
	for (int32 i = 0; i < Count; ++i)
	{
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			Source.ParticleData + Source.ParticleIndices[i] * Source.ParticleStride);

		FParticleMeshInstance& Instance = ScratchInstances[i];
		Instance.World = BuildParticleMatrix(Particle) * Source.SimulationToWorld;   // 메시 로컬 → 시뮬레이션 공간 → 월드
		Instance.Color = FVector4(Particle.Color.R, Particle.Color.G, Particle.Color.B, Particle.Color.A);
	}

	RenderCommand::UpdateBufferData(InstanceBuffer.get(), ScratchInstances.GetData(),
		static_cast<uint32>(Count * sizeof(FParticleMeshInstance)));
	UploadedCount = Count;
}

void FDynamicMeshEmitterData::Submit(FRenderQueue& RenderQueue, const FViewContext& View)
{
	if (UsesInstancing())
		SubmitInstanced(RenderQueue, View);
	else
		SubmitPerParticle(RenderQueue, View);
}

void FDynamicMeshEmitterData::SubmitInstanced(FRenderQueue& RenderQueue, const FViewContext& View)
{
	UStaticMesh* Mesh = Source.Mesh;
	if (UploadedCount <= 0 || !Mesh || !InstanceBuffer)
		return;

	// 행렬은 인스턴스 버퍼에 이미 월드로 들어 있다. 패킷 World(b2)는 쓰지 않는다
	const FVector Delta = Source.EmitterWorldOrigin - View.CameraPosition;

	// 섹션 하나 = 패킷 하나 = DrawIndexedInstanced 한 번 (파티클 수와 무관)
	for (const FStaticMeshSection& Section : Mesh->GetMeshData().Sections)
	{
		if (Section.IndexCount == 0)
			continue;

		FRenderPacket& Packet = RenderQueue.AddDefaulted_GetRef();
		Packet.Mesh                     = Mesh;
		Packet.Material                 = Source.Material;
		Packet.StartIndex               = Section.StartIndex;
		Packet.IndexCount               = Section.IndexCount;
		Packet.LODIndex                 = 0;
		Packet.CameraToParticleDistance = Delta.Dot(Delta);
		Packet.InstanceBuffer           = InstanceBuffer.get();
		Packet.InstanceCount            = static_cast<uint32>(UploadedCount);
	}
}

void FDynamicMeshEmitterData::SubmitPerParticle(FRenderQueue& RenderQueue, const FViewContext& View)
{
	UStaticMesh* Mesh = Source.Mesh;
	const int32 Count = Source.ActiveParticleCount;
	if (Count <= 0 || !Mesh)
		return;

	const TArray<FStaticMeshSection>& Sections = Mesh->GetMeshData().Sections;
	for (int32 i = 0; i < Count; ++i)
	{
		const FBaseParticle& Particle = *reinterpret_cast<const FBaseParticle*>(
			Source.ParticleData + Source.ParticleIndices[i] * Source.ParticleStride);

		// 메시 로컬 → 시뮬레이션 공간 → 월드
		const FMatrix World = BuildParticleMatrix(Particle) * Source.SimulationToWorld;
		const FMatrix* Model = RenderQueue.StoreWorldMatrix(World);

		// 반투명 메시 머티리얼일 때 파티클끼리 뒤→앞으로 정렬되도록 파티클마다 거리를 넣는다
		const FVector Delta = World.GetOrigin() - View.CameraPosition;
		const float DistanceSq = Delta.Dot(Delta);

		for (const FStaticMeshSection& Section : Sections)
		{
			UMaterial* Material = Mesh->GetMaterial(Section.MaterialSlotIndex);
			if (!Material || Section.IndexCount == 0)
				continue;

			FRenderPacket& Packet = RenderQueue.AddDefaulted_GetRef();
			Packet.Model                    = Model;
			Packet.Mesh                     = Mesh;   // VB/IB는 메시 LOD0 것을 그대로 쓴다
			Packet.Material                 = Material;
			Packet.StartIndex               = Section.StartIndex;
			Packet.IndexCount               = Section.IndexCount;
			Packet.LODIndex                 = 0;
			Packet.CameraToParticleDistance = DistanceSq;
		}
	}
}

void ParticleStrip::AppendStrip(const TArray<FParticleStripPoint>& Points, const FViewContext& View, float UVTiling,
	TArray<FParticleStripVertex>& OutVertices, TArray<uint32>& OutIndices)
{
	// 1) 바로 앞 점과 거의 같은 위치의 점은 건너뛴다. 길이 0인 구간은 방향을 구할 수 없다
	constexpr float MinSegmentSq = 1e-8f;
	TArray<int32> Used;
	Used.Reserve(Points.Num());
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		if (!Used.IsEmpty())
		{
			const FVector Delta = Points[i].Position - Points[Used.Last()].Position;
			if (Delta.Dot(Delta) < MinSegmentSq)
				continue;
		}
		Used.Add(i);
	}

	const int32 Count = Used.Num();
	if (Count < 2)
		return;   // 점 0개·1개, 또는 시작점과 끝점이 같은 Beam

	// 2) U 좌표용 길이 방향 위치
	//    점마다 Distance가 들어 있으면(리본) 그 값을 그대로 쓴다 → 무늬가 마디에 붙어서 흐르지 않는다
	//    없으면(Beam) 점 사이 거리를 누적한다
	bool bHasStoredDistance = true;
	for (int32 i = 0; i < Count; ++i)
	{
		if (Points[Used[i]].Distance < 0.0f)
		{
			bHasStoredDistance = false;
			break;
		}
	}

	TArray<float> Distances;
	Distances.SetNum(Count, false);
	if (bHasStoredDistance)
	{
		for (int32 i = 0; i < Count; ++i)
			Distances[i] = Points[Used[i]].Distance;
	}
	else
	{
		Distances[0] = 0.0f;
		for (int32 i = 1; i < Count; ++i)
		{
			const FVector Delta = Points[Used[i]].Position - Points[Used[i - 1]].Position;
			Distances[i] = Distances[i - 1] + std::sqrt(Delta.Dot(Delta));
		}
	}
	// 0~1로 늘리는 경우의 기준. 저장된 거리는 첫 점이 0이 아니고 줄어드는 방향일 수도 있어 차이로 계산한다
	const float StartDistance = Distances[0];
	const float TotalLength   = Distances[Count - 1] - StartDistance;

	// 3) 점마다 좌우 정점 2개
	const uint32 BaseVertex = static_cast<uint32>(OutVertices.Num());
	FVector PrevSide(0.0f, 0.0f, 1.0f);
	for (int32 i = 0; i < Count; ++i)
	{
		const FParticleStripPoint& Point = Points[Used[i]];

		// 진행 방향: 양옆 점을 잇는 방향 (끝점은 한쪽만). 이웃 구간의 평균이라 꺾이는 곳이 벌어지지 않는다
		const FVector& Prev = Points[Used[i > 0 ? i - 1 : i]].Position;
		const FVector& Next = Points[Used[i < Count - 1 ? i + 1 : i]].Position;
		const FVector Tangent = (Next - Prev).Normalized();

		// 카메라를 향하는 면: 진행 방향과 시선 방향 모두에 수직인 방향으로 폭을 편다
		const FVector ToCamera = View.bOrthographic
			? FVector(-View.CameraForward.X, -View.CameraForward.Y, -View.CameraForward.Z)
			: (View.CameraPosition - Point.Position).Normalized();
		FVector Side = FVector::Cross(Tangent, ToCamera);
		if (Side.SizeSquared() < 1e-8f)
			Side = PrevSide;   // 띠를 길이 방향 정면에서 볼 때. 이전 점의 방향을 이어 쓴다
		else
			Side = Side.Normalized();
		PrevSide = Side;

		const FVector HalfWidth = Side * (Point.Width * 0.5f);
		const float U = UVTiling > 0.0f
			? Distances[i] * UVTiling
			: (std::abs(TotalLength) > 1e-6f ? (Distances[i] - StartDistance) / TotalLength : 0.0f);
		const FVector4 Color(Point.Color.R, Point.Color.G, Point.Color.B, Point.Color.A);

		OutVertices.Add({ Point.Position - HalfWidth, FVector2(U, 0.0f), Color });
		OutVertices.Add({ Point.Position + HalfWidth, FVector2(U, 1.0f), Color });
	}

	// 4) 구간마다 사각형 하나 = 삼각형 2개
	for (int32 i = 0; i < Count - 1; ++i)
	{
		const uint32 A = BaseVertex + static_cast<uint32>(i) * 2;   // 이 점 왼쪽
		const uint32 B = A + 1;                                     // 이 점 오른쪽
		const uint32 C = A + 2;                                     // 다음 점 왼쪽
		const uint32 D = A + 3;                                     // 다음 점 오른쪽
		OutIndices.Add(A); OutIndices.Add(B); OutIndices.Add(C);
		OutIndices.Add(C); OutIndices.Add(B); OutIndices.Add(D);
	}
}

void FDynamicStripEmitterDataBase::SubmitStrips(FRenderQueue& RenderQueue, const FViewContext& View,
	const TArray<FParticleStripPoint>* Strips, int32 StripCount,
	UMaterial* Material, float UVTiling, const FVector& WorldOrigin)
{
	if (!Material || !Strips || StripCount <= 0 || NextViewIndex >= MaxViewsPerFrame)
		return;

	// 1) 띠마다 끊어서 모은다. 띠끼리는 정점을 공유하지 않으므로 서로 이어지지 않는다
	ScratchVertices.SetNum(0, false);
	ScratchIndices.SetNum(0, false);
	for (int32 s = 0; s < StripCount; ++s)
		ParticleStrip::AppendStrip(Strips[s], View, UVTiling, ScratchVertices, ScratchIndices);

	if (ScratchIndices.IsEmpty())
		return;

	// 2) 이 뷰 전용 버퍼. 여러 뷰를 제출한 뒤 한꺼번에 그리므로 뷰마다 따로 둔다
	if (NextViewIndex >= ViewResources.Num())
		ViewResources.Add(MakeUnique<FViewResources>());
	FViewResources& Resources = *ViewResources[NextViewIndex++];

	// 점 개수가 프레임마다 달라질 수 있으니, 넘칠 때만 두 배로 키워 다시 만든다
	const int32 VertexCount = ScratchVertices.Num();
	const int32 IndexCount  = ScratchIndices.Num();
	if (!Resources.VertexBuffer || VertexCount > Resources.VertexCapacity)
	{
		Resources.VertexCapacity = std::max(VertexCount, Resources.VertexCapacity * 2);
		Resources.VertexBuffer = RenderCommand::CreateDynamicVertexBuffer(
			static_cast<uint32>(Resources.VertexCapacity * sizeof(FParticleStripVertex)),
			static_cast<uint32>(sizeof(FParticleStripVertex)));
	}
	if (!Resources.IndexBuffer || IndexCount > Resources.IndexCapacity)
	{
		Resources.IndexCapacity = std::max(IndexCount, Resources.IndexCapacity * 2);
		Resources.IndexBuffer = RenderCommand::CreateDynamicIndexBuffer(static_cast<uint32>(Resources.IndexCapacity));
	}
	if (!Resources.VertexBuffer || !Resources.IndexBuffer)
		return;

	RenderCommand::UpdateBufferData(Resources.VertexBuffer.get(), ScratchVertices.GetData(),
		static_cast<uint32>(VertexCount * sizeof(FParticleStripVertex)));
	RenderCommand::UpdateBufferData(Resources.IndexBuffer.get(), ScratchIndices.GetData(),
		static_cast<uint32>(IndexCount * sizeof(uint32)));

	// 3) 상수 (b1)
	Resources.Constants.HasTexture       = Material->Textures.IsEmpty() ? 0.0f : 1.0f;
	Resources.Constants.SoftEdge         = 1.0f;
	Resources.Constants.PremultiplyAlpha = Material->BlendState == EBlendState::Additive ? 1.0f : 0.0f;

	// 4) 정점이 이미 월드 좌표라 World(b2)는 단위 행렬. 이미터 하나 = 패킷 하나
	const FVector Delta = WorldOrigin - View.CameraPosition;

	FRenderPacket Packet;
	Packet.Geometry.VertexBuffer    = Resources.VertexBuffer.get();
	Packet.Geometry.IndexBuffer     = Resources.IndexBuffer.get();
	Packet.IndexCount               = static_cast<uint32>(IndexCount);
	Packet.Material                 = Material;
	Packet.MaterialParamData        = &Resources.Constants;
	Packet.MaterialParamDataSize    = sizeof(FParticleStripConstants);
	Packet.CameraToParticleDistance = Delta.Dot(Delta);
	RenderQueue.Add(Packet);
}

void FDynamicBeamEmitterData::Submit(FRenderQueue& RenderQueue, const FViewContext& View)
{
	SubmitStrips(RenderQueue, View, &Source.Points, 1, Source.Material, Source.UVTiling, Source.EmitterWorldOrigin);
}

void FDynamicRibbonEmitterData::Submit(FRenderQueue& RenderQueue, const FViewContext& View)
{
	SubmitStrips(RenderQueue, View, &Source.Points, 1, Source.Material, Source.UVTiling, Source.EmitterWorldOrigin);
}
