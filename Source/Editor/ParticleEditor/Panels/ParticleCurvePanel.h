#pragma once

#include "ParticleEditorPanel.h"
#include <unordered_map>

class UCurveFloat;

class FParticleCurvePanel : public FParticleEditorPanel
{
public:
	explicit FParticleCurvePanel(FParticleEditor& InEditor);

private:
	enum class ETangentHandle { None, Arrive, Leave };

	void DrawContent() override;
	void DrawGraph(UCurveFloat& Curve);
	void DrawKeyProperties(UCurveFloat& Curve);

	const UCurveFloat* SelectedCurve = nullptr;
	int32 SelectedKeyId = -1;
	int32 DraggedKeyId = -1;
	ETangentHandle DraggedTangent = ETangentHandle::None;
	std::unordered_map<int32, float> TangentHandleLengths;
	float DragMinValue = 0.0f;
	float DragMaxValue = 1.0f;
	float ViewZoom = 1.0f;
	float ViewMinTime = 0.0f;
	float ViewMinValue = 0.0f;
	float ViewMaxValue = 1.0f;
	float GridMinValue = 0.0f;
	float GridMaxValue = 1.0f;
};
