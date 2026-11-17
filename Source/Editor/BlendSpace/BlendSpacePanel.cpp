#include "EnginePCH.h"
#include "Editor/BlendSpace/BlendSpacePanel.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Classes/Animation/BlendSpace.h"
#include "Engine/Classes/Animation/BlendSpaceAnimInstance.h"
#include "Engine/Classes/Animation/CharacterAnimInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectHash.h"

void FBlendSpacePanel::SetPreviewComponent(USkeletalMeshComponent* Component)
{
	PreviewComponent = Component;
	PreviewUUID = Component ? Component->GetUUID() : 0;
}

void FBlendSpacePanel::SetPIEComponent(USkeletalMeshComponent* Component)
{
	PIEComponent = Component;
	PIEUUID = Component ? Component->GetUUID() : 0;
}

USkeletalMeshComponent* FBlendSpacePanel::GetLiveComponent(USkeletalMeshComponent* Component, uint32 UUID) const
{
	uint32 LiveUUID = 0;
	return Component && TryGetLiveObjectUUID(Component, nullptr, LiveUUID) && LiveUUID == UUID
		? Component : nullptr;
}

USkeletalMeshComponent* FBlendSpacePanel::GetPreviewComponent() const
{
	return GetLiveComponent(PreviewComponent, PreviewUUID);
}

UBlendSpaceAnimInstance* FBlendSpacePanel::GetPreviewInstance() const
{
	auto* Component = GetPreviewComponent();
	return Component ? dynamic_cast<UBlendSpaceAnimInstance*>(Component->GetAnimInstance()) : nullptr;
}

UBlendSpaceAnimInstance* FBlendSpacePanel::GetFallbackInstance() const
{
	if (IsPlayingQuery && IsPlayingQuery())
	{
		if (auto* Component = GetLiveComponent(PIEComponent, PIEUUID))
			if (auto* Instance = dynamic_cast<UBlendSpaceAnimInstance*>(Component->GetAnimInstance())) return Instance;
	}
	return GetPreviewInstance();
}

void FBlendSpacePanel::ClearPreview()
{
	SetPreviewComponent(nullptr);
	SetPIEComponent(nullptr);
}

void FBlendSpacePanel::Tick(float DeltaTime)
{
	if (!IsOpen() || (IsPlayingQuery && IsPlayingQuery())) return;
	if (auto* Component = GetPreviewComponent(); Component && Component->GetWorld() &&
		Component->GetWorld()->GetWorldType() == EWorldType::Editor)
		Component->TickComponent(DeltaTime);
}

void FBlendSpacePanel::OnRender()
{
	bool bOpen = IsOpen();
	ImGui::SetNextWindowSize(ImVec2(520, 310), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(GetPanelName(), &bOpen))
	{
		AActor* Selected = SelectedActorQuery ? SelectedActorQuery() : nullptr;
		USkeletalMeshComponent* SkelComp = nullptr;
		if (Selected)
		{
			for (UActorComponent* Candidate : Selected->GetComponents())
			{
				if ((SkelComp = Cast<USkeletalMeshComponent>(Candidate))) break;
			}
		}
		UBlendSpaceAnimInstance* FallbackInst = GetFallbackInstance();
		FAnimNode_BlendSpacePlayer* BSPlayer = nullptr;
		float* SpeedPtr = nullptr;
		if (SkelComp && SkelComp->GetAnimInstance())
		{
			if (auto* BSInst = dynamic_cast<UBlendSpaceAnimInstance*>(SkelComp->GetAnimInstance()))
			{
				BSPlayer = &BSInst->BlendSpacePlayer;
				SpeedPtr = &BSInst->Speed;
			}
			else if (auto* CharInst = Cast<UCharacterAnimInstance>(SkelComp->GetAnimInstance()))
			{
				if (FAnimNode_BlendSpacePlayer* LocBS = CharInst->GetLocomotionBlendSpace())
				{
					BSPlayer = LocBS;
					SpeedPtr = &LocBS->X;
				}
			}
		}
		if (!BSPlayer && FallbackInst)
		{
			BSPlayer = &FallbackInst->BlendSpacePlayer;
			SpeedPtr = &FallbackInst->Speed;
		}

		if (BSPlayer && SpeedPtr)
		{
			UBlendSpace* BS = BSPlayer->BlendSpace.get();
			const float MinX = (BS && BS->SampleData.Num() > 0) ? BS->SampleData[0].SamepleVector.X : 0.0f;
			const float MaxX = (BS && BS->SampleData.Num() > 0) ? BS->SampleData[BS->SampleData.Num() - 1].SamepleVector.X : 600.0f;
			const float RangeX = std::max(MaxX - MinX, 1.0f);

			ImGui::Text("1D Blend Space Canvas (Click & Drag to Preview)");

			// 캔버스 크기 및 위치 계산
			const float CanvasWidth = ImGui::GetContentRegionAvail().x;
			const float CanvasHeight = 110.0f;
			const ImVec2 CanvasSize(CanvasWidth, CanvasHeight);
			const ImVec2 CanvasP0 = ImGui::GetCursorScreenPos();
			const ImVec2 CanvasP1 = ImVec2(CanvasP0.x + CanvasSize.x, CanvasP0.y + CanvasSize.y);
			const float PaddingX = 24.0f;
			const float UsableWidth = std::max(CanvasSize.x - PaddingX * 2.0f, 10.0f);
			const float RailY = CanvasP0.y + CanvasHeight * 0.5f;

			ImDrawList* DrawList = ImGui::GetWindowDrawList();

			// 투명 버튼으로 마우스 이벤트 캡처
			ImGui::InvisibleButton("##BSCanvas", CanvasSize, ImGuiButtonFlags_MouseButtonLeft);
			const bool bCanvasHovered = ImGui::IsItemHovered();
			const bool bCanvasActive = ImGui::IsItemActive();

			// 마우스 클릭 또는 드래그 시 Speed 값 갱신
			if (bCanvasActive)
			{
				const float MouseX = ImGui::GetIO().MousePos.x;
				const float ClampedMouseX = std::clamp(MouseX, CanvasP0.x + PaddingX, CanvasP0.x + PaddingX + UsableWidth);
				const float NormalizedRatio = (ClampedMouseX - (CanvasP0.x + PaddingX)) / UsableWidth;
				*SpeedPtr = MinX + NormalizedRatio * RangeX;
			}

			// 1. 배경 및 테두리
			DrawList->AddRectFilled(CanvasP0, CanvasP1, IM_COL32(24, 24, 27, 255), 6.0f);
			DrawList->AddRect(CanvasP0, CanvasP1, IM_COL32(65, 65, 75, 255), 6.0f);

			// 2. 그리드 수직선 및 수치 눈금 라벨
			const int32 NumDivisions = 4;
			for (int32 i = 0; i <= NumDivisions; ++i)
			{
				const float Frac = static_cast<float>(i) / static_cast<float>(NumDivisions);
				const float LineVal = MinX + Frac * RangeX;
				const float LineX = CanvasP0.x + PaddingX + Frac * UsableWidth;

				DrawList->AddLine(ImVec2(LineX, CanvasP0.y + 4.0f), ImVec2(LineX, CanvasP1.y - 4.0f), IM_COL32(45, 45, 52, 255), 1.0f);

				char LabelBuf[32];
				snprintf(LabelBuf, sizeof(LabelBuf), "%.1f", LineVal);
				DrawList->AddText(ImVec2(LineX - 8.0f, CanvasP1.y - 18.0f), IM_COL32(130, 130, 140, 255), LabelBuf);
			}

			// 3. 메인 트랙 레일 라인
			DrawList->AddLine(ImVec2(CanvasP0.x + PaddingX, RailY), ImVec2(CanvasP0.x + PaddingX + UsableWidth, RailY), IM_COL32(80, 80, 95, 255), 2.0f);

			// 4. 등록된 샘플 포인트 및 가중치 링
			const auto& Cache = BSPlayer->BlendSampleDataCache;
			if (BS)
			{
				for (int32 i = 0; i < BS->SampleData.Num(); ++i)
				{
					const auto& Sample = BS->SampleData[i];
					const float SampleRatio = (Sample.SamepleVector.X - MinX) / RangeX;
					const float SampleX = CanvasP0.x + PaddingX + SampleRatio * UsableWidth;

					// 캐시에서 가중치 검색
					float SampleWeight = 0.0f;
					for (const auto& CachedData : Cache)
					{
						if (CachedData.SampleDataIndex == i)
						{
							SampleWeight = CachedData.TotalWeight;
							break;
						}
					}

					// 가중치에 비례하는 활성 원형 링
					if (SampleWeight > 0.001f)
					{
						const float RingRadius = 6.0f + SampleWeight * 16.0f;
						DrawList->AddCircleFilled(ImVec2(SampleX, RailY), RingRadius, IM_COL32(40, 200, 100, static_cast<int>(SampleWeight * 90)));
						DrawList->AddCircle(ImVec2(SampleX, RailY), RingRadius, IM_COL32(60, 240, 120, static_cast<int>(SampleWeight * 220)), 24, 2.0f);
					}

					// 기본 샘플 마커 (원형 포인트)
					DrawList->AddCircleFilled(ImVec2(SampleX, RailY), 5.5f, IM_COL32(50, 130, 240, 255));
					DrawList->AddCircle(ImVec2(SampleX, RailY), 5.5f, IM_COL32(230, 230, 240, 255), 16, 1.5f);

					// 샘플 이름 텍스트
					if (Sample.Animation)
					{
						const std::string FullName = Sample.Animation->GetName();
						const size_t Sep = FullName.find_last_of('/');
						const std::string ShortName = (Sep != std::string::npos) ? FullName.substr(Sep + 1) : FullName;
						const ImVec2 TextSize = ImGui::CalcTextSize(ShortName.c_str());
						DrawList->AddText(ImVec2(SampleX - TextSize.x * 0.5f, RailY - 24.0f), IM_COL32(210, 210, 220, 255), ShortName.c_str());
					}

					// 마우스 호버 시 툴팁
					const ImVec2 MousePos = ImGui::GetIO().MousePos;
					const float DistSq = (MousePos.x - SampleX) * (MousePos.x - SampleX) + (MousePos.y - RailY) * (MousePos.y - RailY);
					if (bCanvasHovered && DistSq <= 120.0f)
					{
						ImGui::SetTooltip("%s\nSpeed: %.1f\nWeight: %.3f", 
							Sample.Animation ? Sample.Animation->GetName().c_str() : "None",
							Sample.SamepleVector.X, SampleWeight);
					}
				}
			}

			// 5. 현재 미리보기 커서 (녹색 수직선 & 십자선 포인트)
			const float ClampedSpeed = std::clamp(*SpeedPtr, MinX, MaxX);
			const float CursorRatio = (ClampedSpeed - MinX) / RangeX;
			const float CursorX = CanvasP0.x + PaddingX + CursorRatio * UsableWidth;

			// 수직 가이드라인
			DrawList->AddLine(ImVec2(CursorX, CanvasP0.y + 4.0f), ImVec2(CursorX, CanvasP1.y - 4.0f), IM_COL32(50, 240, 90, 180), 1.5f);

			// 중심 커서 마커
			DrawList->AddCircleFilled(ImVec2(CursorX, RailY), 7.5f, IM_COL32(40, 245, 80, 255));
			DrawList->AddCircle(ImVec2(CursorX, RailY), 7.5f, IM_COL32(255, 255, 255, 255), 18, 2.0f);

			// 커서 상단 수치 배지
			char CursorBuf[32];
			snprintf(CursorBuf, sizeof(CursorBuf), "%.1f", ClampedSpeed);
			const ImVec2 CursorTextSize = ImGui::CalcTextSize(CursorBuf);
			DrawList->AddRectFilled(ImVec2(CursorX - CursorTextSize.x * 0.5f - 4.0f, CanvasP0.y + 5.0f),
				ImVec2(CursorX + CursorTextSize.x * 0.5f + 4.0f, CanvasP0.y + 5.0f + CursorTextSize.y + 2.0f),
				IM_COL32(20, 20, 20, 220), 3.0f);
			DrawList->AddText(ImVec2(CursorX - CursorTextSize.x * 0.5f, CanvasP0.y + 6.0f), IM_COL32(70, 255, 110, 255), CursorBuf);

			// 6. 하단 미세 조정 슬라이더 및 재생 시간 바
			ImGui::Spacing();
			ImGui::SliderFloat("Speed", SpeedPtr, MinX, MaxX, "%.2f");
			if (GetPreviewInstance() && FallbackInst && BSPlayer == &FallbackInst->BlendSpacePlayer)
			{
				GetPreviewInstance()->Speed = *SpeedPtr;
			}

			const float NormTime = BSPlayer->InternalTimeAccumulator;
			char TimeBuf[64];
			snprintf(TimeBuf, sizeof(TimeBuf), "Time Loop: %.2f", NormTime);
			ImGui::ProgressBar(NormTime, ImVec2(-1, 8), TimeBuf);

			ImGui::Spacing();
			ImGui::TextDisabled("Tip: Drag the green cursor inside the canvas to smoothly blend locomotion.");
		}
		else ImGui::TextDisabled("Select an actor with a BlendSpace animation, or press F8 in the level editor.");
	}
	ImGui::End();
	SetOpen(bOpen);
}
