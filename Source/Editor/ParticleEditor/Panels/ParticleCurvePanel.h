#pragma once

#include "ParticleEditorPanel.h"

class UCurveFloat;

class FParticleCurvePanel : public FParticleEditorPanel
{
public:
	explicit FParticleCurvePanel(FParticleEditor& InEditor);

private:
	void DrawContent() override;
	void DrawGraph(UCurveFloat& Curve);
	void DrawKeyProperties(UCurveFloat& Curve);

	const UCurveFloat* SelectedCurve = nullptr;
	int32 SelectedKeyId = -1;
	int32 DraggedKeyId = -1;
	float DragMinValue = 0.0f;
	float DragMaxValue = 1.0f;
	float ViewZoom = 1.0f;
	float ViewMinTime = 0.0f;
	float ViewMinValue = 0.0f;
	float ViewMaxValue = 1.0f;
};
