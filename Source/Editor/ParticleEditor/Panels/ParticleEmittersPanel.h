#pragma once

#include "ParticleEditorPanel.h"
#include <vector>

class UParticleEmitter;
class UParticleSystem;

class FParticleEmittersPanel : public FParticleEditorPanel
{
public:
	explicit FParticleEmittersPanel(FParticleEditor& InEditor);

protected:
	void DrawContent() override;

private:
	void DrawEmitterName(int32 Index, UParticleEmitter& Emitter);
	bool DrawEmitterMenu(int32 Index, bool bHasLOD);

	const UParticleSystem* RenameSystem = nullptr;
	const UParticleEmitter* RenamingEmitter = nullptr;
	std::vector<char> RenameBuffer;
	bool bFocusRename = false;
};
