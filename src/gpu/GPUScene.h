#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include "scene/Scene.h"

using namespace vk::raii;

class GPUScene : public Scene
{
public:
  GPUScene(Device& device, VmaAllocator& allocator, CommandPool& cmdPool, Queue& queue);
  virtual ~GPUScene();
  virtual void createRayTracingHierarchy() override;

private:
  void createStorageBuffer(Buffer& buffer, VmaAllocation& alloc, void* data, size_t size);

  Device& device;
  VmaAllocator& allocator;
  CommandPool& cmdPool;
  Queue& queue;

  // bottom level acceleration structure
  struct BLAS
  {
    vk::AccelerationStructureKHR handle = nullptr;
    vk::Buffer buffer = nullptr;
    VmaAllocation alloc = nullptr;
  };

  AccelerationStructureKHR accelerationStructure = nullptr;
  Buffer accelerationBuffer = nullptr;
  VmaAllocation accelerationAllocation = nullptr;
  Buffer instanceBuffer = nullptr;
  VmaAllocation instanceAllocation = nullptr;

  std::vector<BLAS> blas;

  Buffer modelBuffer = nullptr;
  VmaAllocation modelAllocation;

  Buffer materialBuffer = nullptr;
  VmaAllocation materialAllocation;

  Buffer positionBuffer = nullptr;
  VmaAllocation positionAllocation;

  Buffer texCoordsBuffer = nullptr;
  VmaAllocation texCoordsAllocation;

  Buffer normalsBuffer = nullptr;
  VmaAllocation normalsAllocation;

  Buffer directionalLightBuffer = nullptr;
  VmaAllocation directionalLightAllocation;

  Buffer pointLightBuffer = nullptr;
  VmaAllocation pointLightAllocation;

  Buffer indexBuffer = nullptr;
  VmaAllocation indexAllocation;
  friend class GPURenderer;
};