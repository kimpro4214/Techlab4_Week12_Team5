#pragma once

#include "ParticleEditorPanel.h"

class FParticleEmittersPanel : public FParticleEditorPanel
{
public:
	explicit FParticleEmittersPanel(FParticleEditor& InEditor);

protected:
	void DrawContent() override;
};
