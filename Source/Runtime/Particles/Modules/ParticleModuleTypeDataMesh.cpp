#include "EnginePCH.h"
#include "ParticleModuleTypeDataMesh.h"

bool UParticleModuleTypeDataMesh::Validate() const
{
	return Mesh != nullptr;
}
