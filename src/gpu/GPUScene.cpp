#include "GPUScene.h"

GPUScene::~GPUScene() {}

void GPUScene::generate()
{
  populateGeometryPools();

  // upload geometry to gpu
  createStorageBuffer(modelBuffer, modelAllocation, refs.data(), refs.size() * sizeof(ModelReference));
  // createStorageBuffer(materialBuffer, materialAllocation, refs.data(), refs.size() * sizeof(ModelReference));
  createStorageBuffer(positionBuffer, positionAllocation, positionPool.data(), positionPool.size() * sizeof(glm::vec3));
  createStorageBuffer(texCoordsBuffer, texCoordsAllocation, texCoordsPool.data(), texCoordsPool.size() * sizeof(glm::vec2));
  createStorageBuffer(normalsBuffer, normalsAllocation, normalsPool.data(), normalsPool.size() * sizeof(glm::vec3));
  createStorageBuffer(directionalLightBuffer, directionalLightAllocation, directionalLights.data(),
                      directionalLights.size() * sizeof(DirectionalLight));
  createStorageBuffer(pointLightBuffer, pointLightAllocation, pointLights.data(), pointLights.size() * sizeof(PointLight));
  createStorageBuffer(indexBuffer, indexAllocation, indicesPool.data(), indicesPool.size() * sizeof(glm::uvec3));

  VkBufferDeviceAddressInfo addrInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = positionBuffer,
  };
  VkDeviceAddress vertexBufferAddr = vkGetBufferDeviceAddress(*device, &addrInfo);
  addrInfo.buffer = indexBuffer;
  VkDeviceAddress indexBufferAddr = vkGetBufferDeviceAddress(*device, &addrInfo);

  std::vector<vk::AccelerationStructureGeometryKHR> geometries(models.size());
  std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometries(models.size());
  std::vector<vk::AccelerationStructureBuildSizesInfoKHR> buildSizes(models.size());
  std::vector<VkBuffer> scratchBuffers(models.size());
  std::vector<VmaAllocation> scratchAllocations(models.size());
  std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRanges(models.size());
  std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> buildRangePointers(models.size());
  blas.resize(models.size());
  for (uint32_t i = 0; i < models.size(); ++i)
  {
    vk::DeviceOrHostAddressConstKHR vertexDataAddress = (vertexBufferAddr + refs[i].positionOffset * sizeof(glm::vec3));
    vk::DeviceOrHostAddressConstKHR indexDataAddress = (indexBufferAddr + refs[i].indicesOffset + sizeof(glm::uvec3));

    geometries[i] = vk::AccelerationStructureGeometryKHR(
        vk::GeometryTypeKHR::eTriangles,
        vk::AccelerationStructureGeometryTrianglesDataKHR(vk::Format::eR32G32B32Sfloat, vertexDataAddress, sizeof(glm::vec3),
                                                          (uint32_t)refs[i].numPositions, vk::IndexType::eUint32, indexDataAddress),
        vk::GeometryFlagBitsKHR::eOpaque);

    buildGeometries[i] = vk::AccelerationStructureBuildGeometryInfoKHR(
        vk::AccelerationStructureTypeKHR::eTopLevel, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        vk::BuildAccelerationStructureModeKHR::eBuild, {}, {}, 1, &geometries[i], nullptr);

    buildSizes[i] = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        .pNext = nullptr,
    };
    const uint32_t primitiveCount = refs[i].numIndices / 3;
    buildSizes[i] =
        device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometries[i], primitiveCount);

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = buildSizes[i].accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    VmaAllocationCreateInfo bufferAllocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    vmaCreateBuffer(allocator, &bufferInfo, &bufferAllocInfo, &blas[i].buffer, &blas[i].alloc, nullptr);

    vk::AccelerationStructureCreateInfoKHR blasInfo({}, blas[i].buffer, 0, buildSizes[i].accelerationStructureSize,
                                                    vk::AccelerationStructureTypeKHR::eBottomLevel);
    blas[i].handle = device.createAccelerationStructureKHR(blasInfo);

    VkBufferCreateInfo scratchInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = buildSizes[i].buildScratchSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    VmaAllocationCreateInfo scratchAllocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    vmaCreateBufferWithAlignment(allocator, &scratchInfo, &scratchAllocInfo, 16, &scratchBuffers[i], &scratchAllocations[i], nullptr);
    addrInfo.buffer = scratchBuffers[i];
    VkDeviceAddress scratchAddr = vkGetBufferDeviceAddress(*device, &addrInfo);
    buildGeometries[i].dstAccelerationStructure = blas[i].handle;
    buildGeometries[i].scratchData.deviceAddress = scratchAddr;

    buildRanges[i] = VkAccelerationStructureBuildRangeInfoKHR{
        .primitiveCount = primitiveCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0,
    };
    buildRangePointers[i] = &buildRanges[i];
  }
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 10);
  CommandBuffer cmdBuffer = std::move(CommandBuffers(device, commandBufferAllocateInfo).front());
  vk::FenceCreateInfo fenceCreateInfo;
  Fence fence = Fence(device, fenceCreateInfo);
  cmdBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
  cmdBuffer.buildAccelerationStructuresKHR(buildGeometries, buildRangePointers);
  cmdBuffer.end();
  vk::SubmitInfo submitInfo;
  queue.submit(submitInfo, fence);
  assert(device.waitForFences({fence}, true, 1000000) == VK_SUCCESS);
}

void GPUScene::createStorageBuffer(VkBuffer& buffer, VmaAllocation& alloc, void* data, size_t size)
{
  VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size = size;
  bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &buffer, &alloc, nullptr);

  vmaCopyMemoryToAllocation(allocator, data, alloc, 0, size);
}
