#pragma once
#include "gpu/GPUScene.h"
#include "scene/Renderer.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vma/vk_mem_alloc.h>

using namespace vk::raii;

struct GPURenderer : public Renderer
{
public:
  GPURenderer();
  virtual ~GPURenderer();
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
  void createCommands();
  void createDescriptors();
  void createPipeline();

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