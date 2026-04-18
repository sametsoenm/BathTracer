#pragma once

#include "device_types.h"

template <typename T>
struct Record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

using RaygenRecord = Record<EmptyData>;
using MissRecord = Record<MissData>;
using HitgroupRecord = Record<HitGroupData>;
