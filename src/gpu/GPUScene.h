#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vma/vk_mem_alloc.h>
#include "scene/Scene.h"

using namespace vk::raii;

class GPUScene : public Scene
{
public:
  GPUScene(Device& device, VmaAllocator& allocator, CommandPool& cmdPool);
  virtual ~GPUScene();
  virtual void generate() override;

private:
  void createStorageBuffer(VkBuffer& buffer, VmaAllocation& alloc, void* data, size_t size);

  Device& device;
  VmaAllocator& allocator;
  CommandPool& cmdPool;
  Queue& queue;

  // bottom level acceleration structure
  struct BLAS
  {
    vk::AccelerationStructureKHR handle;
    VkBuffer buffer;
    VmaAllocation alloc;
  };

  AccelerationStructureKHR accelerationStructure;
  VmaAllocation accelerationAllocation;

  std::vector<BLAS> blas;

  VkBuffer modelBuffer;
  VmaAllocation modelAllocation;

  VkBuffer materialBuffer;
  VmaAllocation materialAllocation;

  VkBuffer positionBuffer;
  VmaAllocation positionAllocation;

  VkBuffer texCoordsBuffer;
  VmaAllocation texCoordsAllocation;

  VkBuffer normalsBuffer;
  VmaAllocation normalsAllocation;

  VkBuffer directionalLightBuffer;
  VmaAllocation directionalLightAllocation;

  VkBuffer pointLightBuffer;
  VmaAllocation pointLightAllocation;

  VkBuffer indexBuffer;
  VmaAllocation indexAllocation;
  friend class GPURenderer;
};