#pragma once

#include "ParticleEditorPanel.h"

class FParticleDetailsPanel : public FParticleEditorPanel
{
public:
	explicit FParticleDetailsPanel(FParticleEditor& InEditor);

protected:
	void DrawContent() override;
};
