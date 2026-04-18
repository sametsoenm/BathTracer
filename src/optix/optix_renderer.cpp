#include "optix_renderer.h"

#include <optix.h>
#include <optix_stubs.h>
#include <optix_stack_size.h>

#include <cuda.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>

#include <vector>

#include "device_types.h"
#include "util/io.h"
#include "optix/check.h"
#include "optix/records.h"

OptixRenderer::OptixRenderer(OptixScene* scene) : _scene(scene) {

}

void OptixRenderer::init(const uint32_t width, const uint32_t height) {
    createContext();
    createModules();
    createProgramGroups();
    createPipeline();
    createSbt();
    loadEnvironment();
    buildMesh();
    allocateBuffers(width, height);
}

void OptixRenderer::render() {
    if (!_isRendering)
        return;
    
    updateParams();
    launch();
}

static void optixLogCallback(unsigned int level, const char* tag, const char* message, void*)
{
    std::cerr << "[" << level << "][" << (tag ? tag : "") << "] "
        << (message ? message : "") << "\n";
}

void OptixRenderer::createContext() {

    CUDA_CHECK(cuInit(0));
    CUdevice cuDevice = 0;
    CUDA_CHECK(cuDeviceGet(&cuDevice, 0));
    CUDA_CHECK(cuCtxCreate(&_state.cudaContext, 0, cuDevice));

    OPTIX_CHECK(optixInit());
    OptixDeviceContextOptions options = {};
    options.logCallbackFunction = &optixLogCallback;
    options.logCallbackLevel = 4;
    OPTIX_CHECK(optixDeviceContextCreate(_state.cudaContext, &options, &_state.context));

    CUDA_CHECK(cuStreamCreate(&_state.stream, CU_STREAM_DEFAULT));
}

void OptixRenderer::createModules() {

    const std::string ptx = io::readTextFile(DEVICE_PTX_PATH);

    OptixModuleCompileOptions moduleCompileOptions = {};
    moduleCompileOptions.maxRegisterCount = OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT;
    moduleCompileOptions.optLevel = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
    moduleCompileOptions.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_MINIMAL;

    _state.pipelineCompileOptions.usesMotionBlur = false;
    _state.pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    _state.pipelineCompileOptions.numPayloadValues = PAYLOAD_VALUE_COUNT;
    _state.pipelineCompileOptions.numAttributeValues = 2;
    _state.pipelineCompileOptions.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
    _state.pipelineCompileOptions.pipelineLaunchParamsVariableName = "params";

    char log[2048];
    size_t logSize = sizeof(log);

    OPTIX_CHECK(optixModuleCreate(
        _state.context,
        &moduleCompileOptions,
        &_state.pipelineCompileOptions,
        ptx.c_str(),
        ptx.size(),
        log,
        &logSize,
        &_state.module
    ));

    if (logSize > 1)
    {
        std::cout << "OptiX module log:\n" << log << "\n";
    }
}

void OptixRenderer::createProgramGroups() {
    OptixProgramGroupOptions programGroupOptions = {};


    // --------------------------------------------------
    OptixProgramGroupDesc raygenDesc = {};
    raygenDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    raygenDesc.raygen.module = _state.module;
    raygenDesc.raygen.entryFunctionName = "__raygen__rg";

    char log[2048];
    size_t logSize = sizeof(log);

    OPTIX_CHECK(optixProgramGroupCreate(
        _state.context,
        &raygenDesc,
        1,
        &programGroupOptions,
        log,
        &logSize,
        &_state.raygenPG
    ));

    if (logSize > 1)
        std::cout << "Raygen PG log:\n" << log << "\n";


    // --------------------------------------------------
    OptixProgramGroupDesc missDesc = {};
    missDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    missDesc.miss.module = _state.module;
    missDesc.miss.entryFunctionName = "__miss__ms";

    logSize = sizeof(log);
    OPTIX_CHECK(optixProgramGroupCreate(
        _state.context,
        &missDesc,
        1,
        &programGroupOptions,
        log,
        &logSize,
        &_state.missPG
    ));

    if (logSize > 1)
        std::cout << "Miss PG log:\n" << log << "\n";


    // --------------------------------------------------
    OptixProgramGroupDesc hitgroupDesc = {};
    hitgroupDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitgroupDesc.hitgroup.moduleCH = _state.module;
    hitgroupDesc.hitgroup.entryFunctionNameCH = "__closesthit__ch";
    hitgroupDesc.hitgroup.moduleAH = nullptr;
    hitgroupDesc.hitgroup.entryFunctionNameAH = nullptr;
    hitgroupDesc.hitgroup.moduleIS = nullptr;
    hitgroupDesc.hitgroup.entryFunctionNameIS = nullptr;

    logSize = sizeof(log);
    OPTIX_CHECK(optixProgramGroupCreate(
        _state.context,
        &hitgroupDesc,
        1,
        &programGroupOptions,
        log,
        &logSize,
        &_state.hitgroupPG
    ));

    if (logSize > 1)
        std::cout << "Hitgroup PG log:\n" << log << "\n";
}

void OptixRenderer::createPipeline() {

    OptixProgramGroup programGroups[] =
    {
        _state.raygenPG,
        _state.missPG,
        _state.hitgroupPG
    };

    OptixPipelineLinkOptions pipelineLinkOptions = {};
    pipelineLinkOptions.maxTraceDepth = 1;

    char log[2048];
    size_t logSize = sizeof(log);

    OPTIX_CHECK(optixPipelineCreate(
        _state.context,
        &_state.pipelineCompileOptions,
        &pipelineLinkOptions,
        programGroups,
        sizeof(programGroups) / sizeof(programGroups[0]),
        log,
        &logSize,
        &_state.pipeline
    ));

    if (logSize > 1)
    {
        std::cout << "Pipeline log:\n" << log << "\n";
    }

    OptixStackSizes stackSizes = {};

    OPTIX_CHECK(optixUtilAccumulateStackSizes(_state.raygenPG, &stackSizes,   _state.pipeline));
    OPTIX_CHECK(optixUtilAccumulateStackSizes(_state.missPG, &stackSizes,     _state.pipeline));
    OPTIX_CHECK(optixUtilAccumulateStackSizes(_state.hitgroupPG, &stackSizes, _state.pipeline));

    uint32_t directCallableStackSizeFromTraversal = 0;
    uint32_t directCallableStackSizeFromState = 0;
    uint32_t continuationStackSize = 0;

    OPTIX_CHECK(optixUtilComputeStackSizes(
        &stackSizes,
        pipelineLinkOptions.maxTraceDepth,
        0,
        0,
        &directCallableStackSizeFromTraversal,
        &directCallableStackSizeFromState,
        &continuationStackSize
    ));

    OPTIX_CHECK(optixPipelineSetStackSize(
        _state.pipeline,
        directCallableStackSizeFromTraversal,
        directCallableStackSizeFromState,
        continuationStackSize,
        1 // maxTraversableDepth
    ));
}

void OptixRenderer::createSbt() {

    RaygenRecord raygenRecord = {};
    MissRecord missRecord = {};
    OPTIX_CHECK(optixSbtRecordPackHeader(_state.raygenPG, &raygenRecord));
    OPTIX_CHECK(optixSbtRecordPackHeader(_state.missPG, &missRecord));

    const size_t recordCount = 1;
    std::vector<HitgroupRecord> hitgroupRecords(recordCount);
    for (size_t i = 0; i < recordCount; i++) {
        OPTIX_CHECK(optixSbtRecordPackHeader(_state.hitgroupPG, &hitgroupRecords[i]));
    }

    CUdeviceptr d_hitgroupRecords = 0;
    CUdeviceptr d_missRecord = 0;
    CUdeviceptr d_raygenRecord = 0;

    CUDA_CHECK(cuMemAlloc(&d_raygenRecord, sizeof(RaygenRecord)));
    CUDA_CHECK(cuMemAlloc(&d_missRecord, sizeof(MissRecord)));
    CUDA_CHECK(cuMemAlloc(&d_hitgroupRecords, recordCount * sizeof(HitgroupRecord)));

    CUDA_CHECK(cuMemcpyHtoD(d_raygenRecord, &raygenRecord, sizeof(RaygenRecord)));
    CUDA_CHECK(cuMemcpyHtoD(d_missRecord, &missRecord, sizeof(MissRecord)));
    CUDA_CHECK(cuMemcpyHtoD(d_hitgroupRecords, hitgroupRecords.data(), recordCount * sizeof(HitgroupRecord)));

    _state.sbt.raygenRecord = d_raygenRecord;
    _state.sbt.missRecordBase = d_missRecord;
    _state.sbt.missRecordStrideInBytes = sizeof(MissRecord);
    _state.sbt.missRecordCount = 1;
    _state.sbt.hitgroupRecordBase = d_hitgroupRecords;
    _state.sbt.hitgroupRecordStrideInBytes = sizeof(HitgroupRecord);
    _state.sbt.hitgroupRecordCount = static_cast<unsigned int>(recordCount);
}

void OptixRenderer::buildMesh() {

    std::vector<float3> vertices;
    vertices.reserve(_scene->geometry().size() * 3);

    for (size_t i = 0; i < _scene->geometry().size(); ++i)
    {
        const auto& g = _scene->geometry()[i];
        vertices.push_back(g.triangle.v0);
        vertices.push_back(g.triangle.v1);
        vertices.push_back(g.triangle.v2);
    }

    buildTriangleGas(vertices);
    buildIas();


    // full geometry data
    if (!_scene->geometry().empty()) {
        CUDA_CHECK(cuMemAlloc(&_state.d_geometry, _scene->geometry().size() * sizeof(GeometryData)));
        CUDA_CHECK(cuMemcpyHtoD(_state.d_geometry, _scene->geometry().data(), _scene->geometry().size() * sizeof(GeometryData)));
        _state.params.geometry = reinterpret_cast<GeometryData*>(_state.d_geometry);
    }

    // light list
    if (!_scene->lights().empty()) {
        CUDA_CHECK(cuMemAlloc(&_state.d_lights, _scene->lights().size() * sizeof(Light)));
        CUDA_CHECK(cuMemcpyHtoD(_state.d_lights, _scene->lights().data(), _scene->lights().size() * sizeof(Light)));
        _state.params.lights = reinterpret_cast<Light*>(_state.d_lights);
        _state.params.numLights = _scene->lights().size();
    }
    else {
        _state.params.numLights = 0;
    }

    // material list
    if (!_scene->mats().empty()) {
        CUDA_CHECK(cuMemAlloc(&_state.d_mats, _scene->mats().size() * sizeof(Material)));
        CUDA_CHECK(cuMemcpyHtoD(_state.d_mats, _scene->mats().data(), _scene->mats().size() * sizeof(Material)));
        _state.params.materials = reinterpret_cast<Material*>(_state.d_mats);
    }

    // load fog settings ; Maybe move to other function
    _state.params.fogEnabled = _scene->fogEnabled();
    _state.params.fogDensity = _scene->fogDensity();
}

void OptixRenderer::buildTriangleGas(const std::vector<float3>& vertices) {

    CUDA_CHECK(cuMemAlloc(&_state.d_vertices, vertices.size() * sizeof(float3)));
    CUDA_CHECK(cuMemcpyHtoD(_state.d_vertices, vertices.data(), vertices.size() * sizeof(float3)));

    const size_t recordCount = 1;
    std::vector<uint32_t> triangleInputFlags(recordCount, OPTIX_GEOMETRY_FLAG_NONE);

    OptixBuildInput triangleInput = {};
    triangleInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    triangleInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    triangleInput.triangleArray.vertexStrideInBytes = sizeof(float3);
    triangleInput.triangleArray.numVertices = static_cast<unsigned int>(vertices.size());
    triangleInput.triangleArray.vertexBuffers = &_state.d_vertices;

    triangleInput.triangleArray.sbtIndexOffsetBuffer = 0;
    triangleInput.triangleArray.sbtIndexOffsetSizeInBytes = 0;
    triangleInput.triangleArray.sbtIndexOffsetStrideInBytes = 0;

    triangleInput.triangleArray.flags = triangleInputFlags.data();
    triangleInput.triangleArray.numSbtRecords = recordCount;

    OptixAccelBuildOptions accelOptions = {};
    accelOptions.buildFlags = OPTIX_BUILD_FLAG_NONE;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

    OptixAccelBufferSizes gasBufferSizes = {};
    OPTIX_CHECK(optixAccelComputeMemoryUsage(
        _state.context,
        &accelOptions,
        &triangleInput,
        1,
        &gasBufferSizes
    ));

    CUdeviceptr d_gasTempBuffer = 0;

    CUDA_CHECK(cuMemAlloc(&d_gasTempBuffer, gasBufferSizes.tempSizeInBytes));
    CUDA_CHECK(cuMemAlloc(&_state.d_triangleGasOutputBuffer, gasBufferSizes.outputSizeInBytes));

    OPTIX_CHECK(optixAccelBuild(
        _state.context,
        _state.stream, // stream
        &accelOptions,
        &triangleInput,
        1,
        d_gasTempBuffer,
        gasBufferSizes.tempSizeInBytes,
        _state.d_triangleGasOutputBuffer,
        gasBufferSizes.outputSizeInBytes,
        &_state.triangleGasHandle,
        nullptr,
        0
    ));

    CUDA_CHECK(cuStreamSynchronize(0));
    CUDA_CHECK(cuMemFree(d_gasTempBuffer));
}

void OptixRenderer::buildIas()
{
    std::vector<OptixInstance> instances;

    const float identity[12] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };
    
    if (_state.triangleGasHandle != 0) {
        OptixInstance tri = {};
        std::memcpy(tri.transform, identity, sizeof(identity));
        tri.instanceId = 0;
        tri.sbtOffset = 0;
        tri.visibilityMask = 255;
        tri.flags = OPTIX_INSTANCE_FLAG_NONE;
        tri.traversableHandle = _state.triangleGasHandle;
        instances.push_back(tri);
    }

    CUDA_CHECK(cuMemAlloc(&_state.d_instances,
        instances.size() * sizeof(OptixInstance)));
    CUDA_CHECK(cuMemcpyHtoD(_state.d_instances,
        instances.data(),
        instances.size() * sizeof(OptixInstance)));

    OptixBuildInput iasInput = {};
    iasInput.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES;
    iasInput.instanceArray.instances = _state.d_instances;
    iasInput.instanceArray.numInstances = static_cast<unsigned int>(instances.size());

    OptixAccelBuildOptions accelOptions = {};
    accelOptions.buildFlags = OPTIX_BUILD_FLAG_NONE;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

    OptixAccelBufferSizes bufferSizes = {};
    OPTIX_CHECK(optixAccelComputeMemoryUsage(
        _state.context,
        &accelOptions,
        &iasInput,
        1,
        &bufferSizes
    ));

    CUdeviceptr d_tempBuffer = 0;
    CUDA_CHECK(cuMemAlloc(&d_tempBuffer, bufferSizes.tempSizeInBytes));
    CUDA_CHECK(cuMemAlloc(&_state.d_iasOutputBuffer, bufferSizes.outputSizeInBytes));

    OPTIX_CHECK(optixAccelBuild(
        _state.context,
        _state.stream,
        &accelOptions,
        &iasInput,
        1,
        d_tempBuffer,
        bufferSizes.tempSizeInBytes,
        _state.d_iasOutputBuffer,
        bufferSizes.outputSizeInBytes,
        &_state.iasHandle,
        nullptr,
        0
    ));

    CUDA_CHECK(cuStreamSynchronize(0));
    CUDA_CHECK(cuMemFree(d_tempBuffer));
}

void OptixRenderer::allocateBuffers(const uint32_t width, const uint32_t height) {
    _state.params.width = width;
    _state.params.height = height;
    _imageBuffer.resize(width * height);

    // image buffers
    const size_t imageSize = width * height * sizeof(uchar4);
    const size_t accumSize = width * height * sizeof(float4);
    CUDA_CHECK(cuMemAlloc(&_state.d_image, imageSize));
    CUDA_CHECK(cuMemAlloc(&_state.d_accumBuffer, accumSize));
    _state.params.image = reinterpret_cast<uchar4*>(_state.d_image);
    _state.params.accumBuffer = reinterpret_cast<float4*>(_state.d_accumBuffer);
    _state.params.subFrameIdx = 0u;
    _state.params.samplesPerFrame = settings::SAMPLES_PER_FRAME;
    _state.params.maxDepth = settings::MAX_RAY_DEPTH;

    // launchparams
    CUDA_CHECK(cuMemAlloc(&_state.d_params, sizeof(LaunchParams)));
}

void OptixRenderer::loadEnvironment() {

    const auto& envTex = _scene->environment().map;
    const size_t width = envTex.width();
    const size_t height = envTex.height();

    std::vector<float4> pixelsGPU;
    pixelsGPU.reserve(width * height);
    for (const glm::vec3& p : envTex.pixels())
    {
        pixelsGPU.push_back(make_float4(p.x, p.y, p.z, 1.0f));
    }

    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<float4>();

    CUDA_RT_CHECK(cudaMallocArray(
        &_state.d_envArray,
        &channelDesc,
        width,
        height
    ));

    CUDA_RT_CHECK(cudaMemcpy2DToArray(
        _state.d_envArray,
        0, 0,
        pixelsGPU.data(),
        width * sizeof(float4),
        width * sizeof(float4),
        height,
        cudaMemcpyHostToDevice
    ));

    cudaResourceDesc resDesc = {};
    resDesc.resType = cudaResourceTypeArray;
    resDesc.res.array.array = _state.d_envArray;

    cudaTextureDesc texDesc = {};
    texDesc.addressMode[0] = cudaAddressModeWrap;   // u
    texDesc.addressMode[1] = cudaAddressModeClamp;  // v
    texDesc.filterMode = cudaFilterModeLinear;
    texDesc.readMode = cudaReadModeElementType;
    texDesc.normalizedCoords = 1;

    CUDA_RT_CHECK(cudaCreateTextureObject(
        &_state.envTexture,
        &resDesc,
        &texDesc,
        nullptr
    ));

    _state.params.envmap = _state.envTexture;
    _state.params.envmapEnabled = _scene->envmapEnabled();
}

void OptixRenderer::reloadScene() {

    // destroy old
    CUDA_CHECK(cuMemFree(_state.d_lights));
    _state.d_lights = 0;
    CUDA_CHECK(cuMemFree(_state.d_mats));
    _state.d_mats = 0;
    CUDA_CHECK(cuMemFree(_state.d_vertices));
    _state.d_vertices = 0;
    CUDA_CHECK(cuMemFree(_state.d_geometry));
    _state.d_geometry = 0;
    CUDA_CHECK(cuMemFree(_state.d_instances));
    _state.d_instances = 0;
    CUDA_CHECK(cuMemFree(_state.d_triangleGasOutputBuffer));
    _state.d_triangleGasOutputBuffer = 0;
    CUDA_CHECK(cuMemFree(_state.d_iasOutputBuffer));
    _state.d_iasOutputBuffer = 0;

    _state.triangleGasHandle = 0;
    _state.iasHandle = 0;
    _state.params.traversable = 0;

    _state.params.fogEnabled = _scene->fogEnabled();
    _state.params.fogDensity = _scene->fogDensity();

    buildMesh();
    reloadEnvironment();

}

void OptixRenderer::reloadEnvironment() {

    // destroy old
    CUDA_RT_CHECK(cudaDestroyTextureObject(_state.envTexture));
    _state.envTexture = 0;
    CUDA_RT_CHECK(cudaFreeArray(_state.d_envArray));
    _state.d_envArray = 0;

    // load new
    loadEnvironment();

}

void OptixRenderer::updateParams() {

    _state.params.camera = _scene->cam().getDeviceData();
    _state.params.traversable = _state.iasHandle;
    _state.params.envmapEnabled = _scene->envmapEnabled();

    // maybe only on demand?
    _state.params.fogEnabled = _scene->fogEnabled();
    _state.params.fogDensity = _scene->fogDensity();
    _state.params.fogMode = _fogMode;
}

void OptixRenderer::launch() {
    
    CUDA_CHECK(cuMemcpyHtoD(
        _state.d_params,
        &_state.params,
        sizeof(LaunchParams)
    ));

    OPTIX_CHECK(optixLaunch(
        _state.pipeline,
        _state.stream, // CUstream
        _state.d_params,
        sizeof(LaunchParams),
        &_state.sbt,
        _state.params.width,
        _state.params.height,
        1
    ));

    CUDA_CHECK(cuStreamSynchronize(0));

    const size_t imageSize = _state.params.width * _state.params.height * sizeof(uchar4);
    CUDA_CHECK(cuMemcpyDtoH(
        _imageBuffer.data(),
        _state.d_image,
        imageSize
    ));

    _state.params.subFrameIdx++;
}

void OptixRenderer::shutdown() {

    CUDA_RT_CHECK(cudaDestroyTextureObject(_state.envTexture));
    CUDA_RT_CHECK(cudaFreeArray(_state.d_envArray));
    CUDA_CHECK(cuMemFree(_state.d_lights));
    CUDA_CHECK(cuMemFree(_state.d_mats));
    CUDA_CHECK(cuMemFree(_state.d_vertices));
    CUDA_CHECK(cuMemFree(_state.d_geometry));
    CUDA_CHECK(cuMemFree(_state.d_instances));
    CUDA_CHECK(cuMemFree(_state.d_triangleGasOutputBuffer));
    CUDA_CHECK(cuMemFree(_state.d_iasOutputBuffer));

    CUDA_CHECK(cuMemFree(_state.sbt.hitgroupRecordBase));
    CUDA_CHECK(cuMemFree(_state.sbt.missRecordBase));
    CUDA_CHECK(cuMemFree(_state.sbt.raygenRecord));

    CUDA_CHECK(cuMemFree(_state.d_accumBuffer));
    CUDA_CHECK(cuMemFree(_state.d_image));
    CUDA_CHECK(cuMemFree(_state.d_params));

    OPTIX_CHECK(optixPipelineDestroy(_state.pipeline));
    OPTIX_CHECK(optixProgramGroupDestroy(_state.hitgroupPG));
    OPTIX_CHECK(optixProgramGroupDestroy(_state.missPG));
    OPTIX_CHECK(optixProgramGroupDestroy(_state.raygenPG));
    OPTIX_CHECK(optixModuleDestroy(_state.module));
    CUDA_CHECK(cuStreamDestroy(_state.stream));
    OPTIX_CHECK(optixDeviceContextDestroy(_state.context));
    CUDA_CHECK(cuCtxDestroy(_state.cudaContext));
}

void OptixRenderer::resetAccumulation() {
    _state.params.subFrameIdx = 0u;
    const size_t accumSize = _state.params.width * _state.params.height * sizeof(float4);
    CUDA_CHECK(cuMemsetD8(_state.d_accumBuffer, 0, accumSize));
}