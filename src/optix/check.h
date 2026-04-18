#pragma once

#include <optix.h>
#include <cuda.h>
#include <iostream>

void checkCuda(CUresult result, const char* call)
{
    if (result != CUDA_SUCCESS)
    {
        const char* name = nullptr;
        const char* desc = nullptr;
        cuGetErrorName(result, &name);
        cuGetErrorString(result, &desc);
        std::cerr << "CUDA error in " << call << ": "
            << (name ? name : "unknown") << " - "
            << (desc ? desc : "no description") << "\n";
        std::exit(1);
    }
}

#define CUDA_CHECK(x) checkCuda((x), #x)

inline void checkCudaRuntime(cudaError_t result, const char* func)
{
    if (result != cudaSuccess)
    {
        std::cerr << "CUDA Runtime Error at " << func << ": "
            << cudaGetErrorString(result) << std::endl;
        exit(1);
    }
}

#define CUDA_RT_CHECK(call) checkCudaRuntime(call, #call)

void checkOptix(OptixResult result, const char* call)
{
    if (result != OPTIX_SUCCESS)
    {
        std::cerr << "OptiX error in " << call << ": " << static_cast<int>(result) << "\n";
        std::exit(1);
    }
}

#define OPTIX_CHECK(x) checkOptix((x), #x)