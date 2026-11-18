#pragma once

#include "Asset/AssetUtils.h"

#include "Core/Types.h"

struct FLODBakeHeader
{
    uint32 Magic = AssetUtils::MakeMagicNumber("SLOD");
    uint32 Version = 1;
    uint64 LOD0Hash = 0;
    uint32 LODCount = 3;
    float ScreenThresholds[3]{};
};

