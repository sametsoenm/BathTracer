#pragma once

#include <optix.h>
#include <optix_stubs.h>
#include <optix_stack_size.h>

#include <cuda.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>
#include <driver_types.h>

#include "device_types.h"
#include "rendersettings.h"
#include "optix_scene.h"

#include <vector>

struct PathTracerState
{
	OptixDeviceContext context = 0;
	CUcontext cudaContext = 0;
	CUstream stream = 0;

	OptixTraversableHandle iasHandle = 0;
	OptixTraversableHandle triangleGasHandle = 0;
	CUdeviceptr d_iasOutputBuffer = 0;
	CUdeviceptr d_triangleGasOutputBuffer = 0;

	CUdeviceptr d_instances = 0;
	CUdeviceptr d_geometry = 0;
	CUdeviceptr d_vertices = 0;
	CUdeviceptr d_lights = 0;
	CUdeviceptr d_mats = 0;
	cudaArray_t d_envArray = 0;
	cudaTextureObject_t envTexture = 0;
	CUdeviceptr d_textures = 0;
	std::vector<cudaArray_t> d_texArrays;
	std::vector<cudaTextureObject_t> textures;

	OptixModule module = 0;
	OptixPipelineCompileOptions pipelineCompileOptions = {};
	OptixPipeline pipeline = 0;

	OptixProgramGroup raygenPG = 0;
	OptixProgramGroup missPG = 0;
	OptixProgramGroup hitgroupPG = 0;

	LaunchParams params;
	CUdeviceptr d_params;

	OptixShaderBindingTable sbt = {};

	CUdeviceptr d_image = 0;
	CUdeviceptr d_accumBuffer = 0;
};

class OptixRenderer {
public:

	OptixRenderer(OptixScene* scene);

	static constexpr unsigned int PAYLOAD_VALUE_COUNT = 20;

	void init(const uint32_t width, const uint32_t height);
	void render();
	void shutdown();

	void resetAccumulation();
	void reloadScene();
	void reloadEnvironment();

	const OptixScene& scene() const { return *_scene; }
	OptixScene& scene() { return *_scene; }
	void setScene(OptixScene* __scene) { _scene = __scene; }
	const std::vector<uchar4>& buffer() const { return _imageBuffer; }
	types::RendererType getType() const { return types::RendererType::OPTIX; }
	bool isRendering() const { return _isRendering; }
	void setIsRendering(bool b) { _isRendering = b; }
	Camera& cam() { return _scene->cam(); }
	unsigned int sampleIndex() { 
		return settings::SAMPLES_PER_FRAME * _state.params.subFrameIdx; 
	}
	FogMode fogMode() const { return _fogMode; }
	void setFogMode(const FogMode mode) { _fogMode = mode; }
	
private:
	void createContext();
	void createModules();
	void createProgramGroups();
	void createPipeline();
	void createSbt();
	void allocateBuffers(const uint32_t width, const uint32_t height);
	void loadEnvironment();
	void loadTextures();
	void updateParams();
	void launch();

	void buildMesh();
	void buildIas();
	void buildTriangleGas(const std::vector<float3>& vertices);

private:
	PathTracerState _state;
	std::vector<uchar4> _imageBuffer;
	OptixScene* _scene;

	bool _isRendering = true;
	FogMode _fogMode = FogMode::SINGLE_SCATTER; 
};