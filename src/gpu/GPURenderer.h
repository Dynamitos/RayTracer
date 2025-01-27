#pragma once
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
  void createDevice();
  void createCommands();
  void createDescriptors();
  void createShaders();

  Context context;
  Instance instance;
  PhysicalDevice physicalDevice;
  Device device;
  Queue queue;
  VmaAllocator allocator;

  uint32_t computeQueueFamily;
  CommandPool cmdPool;
  CommandBuffers cmdBuffers;

  DescriptorSetLayout descriptorLayout;
  DescriptorSet descriptorSet;
  DescriptorPool descriptorPool;
  PipelineLayout pipelineLayout;

  ShaderModule rayGen;
  ShaderModule closestHit;
  ShaderModule miss;

  Pipeline pipeline;

  VkBuffer cameraBuffer;
  VmaAllocation cameraAllocation;

  VkImage radianceAccumulator;
  VmaAllocation radianceAllocation;

  VkImage image;
  VmaAllocation imageAllocation;

  virtual void render(Camera cam, RenderParameter param);
};