#pragma once
#include "gpu/GPUScene.h"
#include "scene/Renderer.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

using namespace vk::raii;

struct GPURenderer : public Renderer
{
public:
  GPURenderer();
  virtual ~GPURenderer();  
  virtual void addPointLight(PointLight point) override { scene->addPointLight(point); }
  virtual void addDirectionalLight(DirectionalLight dir) override { scene->addDirectionalLight(dir); }
  virtual void addModel(PModel model, glm::mat4 transform) override { scene->addModel(std::move(model), transform); }
  virtual void addModels(std::vector<PModel> models, glm::mat4 transform) override { scene->addModels(std::move(models), transform); }
  virtual void generate() override { scene->generate(); }
  virtual void render(Camera cam, RenderParameter param) override;

private:
  struct GPUCamera
  {
    glm::vec3 cameraPosition;
    float f;
    glm::vec3 cameraForward;
    float S_O;
    glm::vec3 fogEmm;
    float ks;
    float A;
    float ka;
  };

  struct SampleParams
  {
    uint32_t pass;
    uint32_t samplesPerPixel;
    uint32_t numDirectionalLights;
    uint32_t numPointLights;
  };
  void createDevice();
  void createDescriptors();
  void createPipeline();

  GPUScene* scene;

  Context context;
  Instance instance = nullptr;
  PhysicalDevice physicalDevice = nullptr;
  Device device = nullptr;
  Queue queue = nullptr;
  VmaAllocator allocator = nullptr;

  vk::PhysicalDeviceAccelerationStructurePropertiesKHR accelerationProperties = {};
  vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = {};

  uint32_t computeQueueFamily = 0;
  CommandPool cmdPool = nullptr;
  CommandBuffers cmdBuffers = nullptr;
  std::vector<Semaphore> semaphores;
  std::vector<Fence> fences;

  DescriptorSetLayout descriptorLayout = nullptr;
  DescriptorSet descriptorSet = nullptr;
  DescriptorPool descriptorPool = nullptr;
  PipelineLayout pipelineLayout = nullptr;

  ShaderModule rayGen = nullptr;
  ShaderModule closestHit = nullptr;
  ShaderModule miss = nullptr;

  Pipeline pipeline = nullptr;

  Buffer rayGenSBT = nullptr;
  vk::StridedDeviceAddressRegionKHR rayGenAddr;
  VmaAllocation rayGenAlloc;

  Buffer closestHitSBT = nullptr;
  vk::StridedDeviceAddressRegionKHR closestHitAddr;
  VmaAllocation closestHitAlloc;

  Buffer missSBT = nullptr;
  vk::StridedDeviceAddressRegionKHR missAddr;
  VmaAllocation missAlloc;

  Buffer cameraBuffer = nullptr;
  VmaAllocation cameraAllocation;

  Image radianceAccumulator = nullptr;
  ImageView radianceView = nullptr;
  VmaAllocation radianceAllocation;

  Image image = nullptr;
  ImageView imageView = nullptr;
  VmaAllocation imageAllocation;

  void uploadToGPU(Buffer& buffer, void* data, size_t size);
};