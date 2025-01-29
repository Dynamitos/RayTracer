#include "GPUScene.h"

GPUScene::GPUScene(Device& device, VmaAllocator& allocator, CommandPool& cmdPool, Queue& queue)
    : device(device), allocator(allocator), cmdPool(cmdPool), queue(queue)
{
}

GPUScene::~GPUScene() {}

void GPUScene::createRayTracingHierarchy()
{
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

  vk::DeviceAddress vertexBufferAddr = device.getBufferAddress(vk::BufferDeviceAddressInfo(positionBuffer));
  vk::DeviceAddress indexBufferAddr = device.getBufferAddress(vk::BufferDeviceAddressInfo(indexBuffer));

  std::vector<vk::AccelerationStructureInstanceKHR> instances(models.size());
  {
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
          vk::AccelerationStructureTypeKHR::eBottomLevel, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
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
      VkBuffer buf;
      vmaCreateBuffer(allocator, &bufferInfo, &bufferAllocInfo, &buf, &blas[i].alloc, nullptr);
      blas[i].buffer = Buffer(device, buf);

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
      vk::DeviceAddress scratchAddr = device.getBufferAddress(vk::BufferDeviceAddressInfo(scratchBuffers[i]));
      buildGeometries[i].dstAccelerationStructure = blas[i].handle;
      buildGeometries[i].scratchData.deviceAddress = scratchAddr;

      buildRanges[i] = VkAccelerationStructureBuildRangeInfoKHR{
          .primitiveCount = primitiveCount,
          .primitiveOffset = 0,
          .firstVertex = 0,
          .transformOffset = 0,
      };
      buildRangePointers[i] = &buildRanges[i];
      vk::DeviceAddress blasAddr = device.getBufferAddress(vk::BufferDeviceAddressInfo(blas[i].buffer));
      instances[i] = vk::AccelerationStructureInstanceKHR({}, i, 0xff, 0, {}, blasAddr);
    }
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 10);
    CommandBuffer cmdBuffer = std::move(CommandBuffers(device, commandBufferAllocateInfo).front());
    cmdBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    cmdBuffer.buildAccelerationStructuresKHR(buildGeometries, buildRangePointers);
    cmdBuffer.end();
    vk::SubmitInfo submitInfo;
    queue.submit(submitInfo);
    device.waitIdle();
  }
  createStorageBuffer(instanceBuffer, instanceAllocation, instances.data(), instances.size());
  vk::DeviceAddress instancesAddress = device.getBufferAddress(vk::BufferDeviceAddressInfo(instanceBuffer));
  vk::AccelerationStructureGeometryKHR geometry(vk::GeometryTypeKHR::eInstances,
                                                vk::AccelerationStructureGeometryInstancesDataKHR(false, {instancesAddress}),
                                                vk::GeometryFlagBitsKHR::eOpaque);
  vk::AccelerationStructureBuildGeometryInfoKHR structureBuildGeometry(vk::AccelerationStructureTypeKHR::eTopLevel,
                                                                       vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
                                                                       vk::BuildAccelerationStructureModeKHR::eBuild, {}, {}, geometry);

  const uint32_t primitiveCount = instances.size();
  auto buildSizes =
      device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, structureBuildGeometry, primitiveCount);

  VkBuffer buffer;
  auto tlasInfo = VkBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = buildSizes.accelerationStructureSize,
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
  };
  auto tlasAlloc = VmaAllocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_AUTO,
  };
  vmaCreateBuffer(allocator, &tlasInfo, &tlasAlloc, &buffer, &accelerationAllocation, nullptr);
  accelerationBuffer = Buffer(device, buffer);

  accelerationStructure = device.createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR(
      {}, accelerationBuffer, 0, buildSizes.accelerationStructureSize, vk::AccelerationStructureTypeKHR::eTopLevel));

  auto scratchInfo = VkBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = buildSizes.buildScratchSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
  };
  auto scratchAllocInfo = VmaAllocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_AUTO,
  };
  VkBuffer scratchBuf;
  VmaAllocation scratchAlloc;
  vmaCreateBufferWithAlignment(allocator, &scratchInfo, &scratchAllocInfo, 64, &scratchBuf, &scratchAlloc, nullptr);
  Buffer scratchBuffer = Buffer(device, scratchBuf);

  vk::DeviceAddress scratchAddr = device.getBufferAddress(vk::BufferDeviceAddressInfo(scratchBuffer));
  vk::AccelerationStructureBuildGeometryInfoKHR buildGeometry(
      vk::AccelerationStructureTypeKHR::eTopLevel, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
      vk::BuildAccelerationStructureModeKHR::eBuild, {}, accelerationStructure, geometry, {}, {scratchAddr});

  vk::AccelerationStructureBuildRangeInfoKHR buildRange(primitiveCount, 0, 0, 0);

  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 10);
  CommandBuffer cmdBuffer = std::move(CommandBuffers(device, commandBufferAllocateInfo).front());
  cmdBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
  cmdBuffer.buildAccelerationStructuresKHR(buildGeometry, {&buildRange});
  cmdBuffer.end();
  vk::SubmitInfo submitInfo;
  queue.submit(submitInfo);
  device.waitIdle();
}

void GPUScene::createStorageBuffer(Buffer& buffer, VmaAllocation& alloc, void* data, size_t size)
{
  if (size == 0)
    return;
  VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size = size;
  bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  VkBuffer buf;
  vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &buf, &alloc, nullptr);
  buffer = Buffer(device, buf);

  VkBufferCreateInfo stagingBufInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  };
  VmaAllocationCreateInfo stagingAllocInfo = {
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
  };
  VkBuffer stagingBuf;
  VmaAllocation stagingAllocation;
  vmaCreateBuffer(allocator, &stagingBufInfo, &stagingAllocInfo, &stagingBuf, &stagingAllocation, nullptr);
  Buffer stagingBuffer = Buffer(device, stagingBuf);

  vmaCopyMemoryToAllocation(allocator, data, stagingAllocation, 0, size);
  CommandBuffer copyCmd =
      std::move(device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1)).front());
  copyCmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
  copyCmd.copyBuffer(stagingBuffer, buffer, vk::BufferCopy(0, 0, size));
  copyCmd.end();
  queue.submit(vk::SubmitInfo({}, {}, *copyCmd, {}));
  device.waitIdle();
}
