#include "GPURenderer.h"
#include "gpu/GPUScene.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_raii.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <slang-com-ptr.h>
#include <slang.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

GPURenderer::GPURenderer()
{
  createDevice();
  scene = new GPUScene(device, allocator, cmdPool, queue);
  createDescriptors();
  createPipeline();
}

GPURenderer::~GPURenderer() {}

void GPURenderer::createDevice()
{
  vk::ApplicationInfo appInfo("RayTracer", 1, "RayTracer", 1, VK_API_VERSION_1_3);
  vk::InstanceCreateInfo instanceCreateInfo({}, &appInfo);
  instance = Instance(context, instanceCreateInfo);
  auto physicalDevices = PhysicalDevices(instance);
  for (auto& dev : physicalDevices)
  {
    for (auto ext : dev.enumerateDeviceExtensionProperties())
    {
      if (std::strcmp(ext.extensionName, vk::KHRRayTracingPipelineExtensionName))
      {
        physicalDevice = dev;
        break;
      }
    }
  }
  auto properties = physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceAccelerationStructurePropertiesKHR,
                                                  vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

  accelerationProperties = properties.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
  rayTracingProperties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

  uint32_t computeQueueFamily = 0;
  auto queueProps = physicalDevice.getQueueFamilyProperties();
  for (uint32_t i = 0; i < queueProps.size(); ++i)
  {
    if (queueProps[i].queueFlags & vk::QueueFlagBits::eCompute)
    {
      computeQueueFamily = i;
      break;
    }
  }
  std::vector<float> queuePriority = {1.0f};
  auto featureChain = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
                                                  vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
  auto features = featureChain.get<vk::PhysicalDeviceFeatures2>();

  vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, computeQueueFamily, queuePriority);
  const char* extensions[] = {vk::KHRAccelerationStructureExtensionName, vk::KHRRayTracingPipelineExtensionName, vk::KHRDeferredHostOperationsExtensionName};
  vk::DeviceCreateInfo deviceCreateInfo({}, deviceQueueCreateInfo, {}, extensions, nullptr, &features);
  device = Device(physicalDevice, deviceCreateInfo);

  queue = Queue(device, computeQueueFamily, 0);

  VmaVulkanFunctions vulkanFunctions = {
      .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = &vkGetDeviceProcAddr,
  };

  VmaAllocatorCreateInfo allocatorCreateInfo = {
      .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = *physicalDevice,
      .device = *device,
      .pVulkanFunctions = &vulkanFunctions,
      .instance = *instance,
      .vulkanApiVersion = VK_API_VERSION_1_3,
  };

  vmaCreateAllocator(&allocatorCreateInfo, &allocator);
  vk::CommandPoolCreateInfo commandPoolCreateInfo({}, computeQueueFamily);
  cmdPool = CommandPool(device, commandPoolCreateInfo);
}

void GPURenderer::createDescriptors()
{
  vk::DescriptorSetLayoutBinding bindings[] = {
      // camera
      vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // scene acceleration structure
      vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eAccelerationStructureKHR, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // radiance accumulator
      vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageImage, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // image
      vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageImage, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // model data
      vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // material data
      vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // positions
      vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // texcoords
      vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // normals
      vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // directional lights
      vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // point lights
      vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
      // index buffer
      vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageBuffer, 1,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR),
  };

  vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, bindings);
  descriptorLayout = DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);

  auto descriptorPoolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
      vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
      vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 2),
      vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 8),
  };
  descriptorPool =
      DescriptorPool(device, vk::DescriptorPoolCreateInfo({vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet}, 4, descriptorPoolSizes));

  // create a PipelineLayout using that DescriptorSetLayout
  vk::PushConstantRange range =
      vk::PushConstantRange(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(SampleParams));
  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, *descriptorLayout, range);
  pipelineLayout = PipelineLayout(device, pipelineLayoutCreateInfo);
}

using namespace slang;

template <typename T> constexpr T align(T size, T alignment) { return (size + alignment - 1) & ~(alignment - 1); }

void GPURenderer::createPipeline()
{
  Slang::ComPtr<IGlobalSession> globalSession;
  //createGlobalSession(globalSession.writeRef());
  TargetDesc targetDesc = {
      .format = SLANG_SPIRV,
      .profile = globalSession->findProfile("glsl_450"),
  };
  const char* searchPaths[] = {"../res/shaders/"};
  SessionDesc sessionDesc = {
      .targets = &targetDesc,
      .targetCount = 1,
      .searchPaths = searchPaths,
      .searchPathCount = 1,
  };
  Slang::ComPtr<ISession> session;
  globalSession->createSession(sessionDesc, session.writeRef());

  Slang::ComPtr<IBlob> diagnostics;
  IModule* raygenModule = session->loadModule("RayGen", diagnostics.writeRef());
  if (diagnostics)
  {
    std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
  }
  Slang::ComPtr<IEntryPoint> rayGenEntry;
  raygenModule->findEntryPointByName("raygen", rayGenEntry.writeRef());

  IModule* closestHitModule = session->loadModule("ClosestHit", diagnostics.writeRef());
  if (diagnostics)
  {
    std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
  }
  Slang::ComPtr<IEntryPoint> closestHitEntry;
  closestHitModule->findEntryPointByName("closestHit", closestHitEntry.writeRef());

  IModule* missModule = session->loadModule("Miss", diagnostics.writeRef());
  if (diagnostics)
  {
    std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
  }
  Slang::ComPtr<IEntryPoint> missEntry;
  missModule->findEntryPointByName("miss", missEntry.writeRef());

  IComponentType* components[] = {raygenModule, rayGenEntry, closestHitModule, closestHitEntry, missModule, missEntry};
  Slang::ComPtr<IComponentType> program;
  session->createCompositeComponentType(components, 6, program.writeRef());

  Slang::ComPtr<IComponentType> linkedProgram;
  program->link(linkedProgram.writeRef(), diagnostics.writeRef());

  Slang::ComPtr<IBlob> rayGenCode;
  linkedProgram->getEntryPointCode(0, 0, rayGenCode.writeRef(), diagnostics.writeRef());

  Slang::ComPtr<IBlob> closestHitCode;
  linkedProgram->getEntryPointCode(1, 0, closestHitCode.writeRef(), diagnostics.writeRef());

  Slang::ComPtr<IBlob> missCode;
  linkedProgram->getEntryPointCode(2, 0, missCode.writeRef(), diagnostics.writeRef());

  rayGen =
      ShaderModule(device, vk::ShaderModuleCreateInfo({}, rayGenCode->getBufferSize(), (const uint32_t*)rayGenCode->getBufferPointer()));

  closestHit = ShaderModule(
      device, vk::ShaderModuleCreateInfo({}, closestHitCode->getBufferSize(), (const uint32_t*)closestHitCode->getBufferPointer()));

  miss = ShaderModule(device, vk::ShaderModuleCreateInfo({}, missCode->getBufferSize(), (const uint32_t*)missCode->getBufferPointer()));

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
  std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

  {
    shaderStages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eRaygenKHR, *rayGen, "main"));
    shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, shaderStages.size() - 1,
                                                                  vk::ShaderUnusedKHR, vk::ShaderUnusedKHR, vk::ShaderUnusedKHR));
  }
  {
    shaderStages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eClosestHitKHR, *closestHit, "main"));

    uint32_t hitIndex = static_cast<uint32_t>(shaderStages.size() - 1);
    uint32_t anyHitIndex = VK_SHADER_UNUSED_KHR;
    uint32_t intersectionIndex = VK_SHADER_UNUSED_KHR;
    // if (hitgroup.anyHitShader != nullptr)
    //{
    //   auto anyHit = hitgroup.anyHitShader.cast<AnyHitShader>();
    //   shaderStages.add(VkPipelineShaderStageCreateInfo{
    //       .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    //       .pNext = nullptr,
    //       .flags = 0,
    //       .stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
    //       .module = anyHit->getModuleHandle(),
    //       .pName = anyHit->getEntryPointName(),
    //       .pSpecializationInfo = nullptr,
    //   });
    // }
    // if (hitgroup.intersectionShader != nullptr)
    //{
    //   auto intersect = hitgroup.intersectionShader.cast<IntersectionShader>();
    //   shaderStages.add(VkPipelineShaderStageCreateInfo{
    //       .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    //       .pNext = nullptr,
    //       .flags = 0,
    //       .stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
    //       .module = intersect->getModuleHandle(),
    //       .pName = intersect->getEntryPointName(),
    //       .pSpecializationInfo = nullptr,
    //   });
    // }
    shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, vk::ShaderUnusedKHR,
                                                                  hitIndex, anyHitIndex, intersectionIndex));
  }
  {
    shaderStages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eMissKHR, *miss, "main"));
    shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, shaderStages.size() - 1,
                                                                  vk::ShaderUnusedKHR, vk::ShaderUnusedKHR, vk::ShaderUnusedKHR));
  }
  pipeline = device.createRayTracingPipelineKHR(
      nullptr, nullptr, vk::RayTracingPipelineCreateInfoKHR({}, shaderStages, shaderGroups, 12, nullptr, nullptr, nullptr, *pipelineLayout));

  const uint32_t handleSize = rayTracingProperties.shaderGroupHandleSize;
  const uint32_t handleSizeAligned = align(rayTracingProperties.shaderGroupHandleSize, rayTracingProperties.shaderGroupHandleAlignment);
  const uint32_t handleAlignment = rayTracingProperties.shaderGroupHandleAlignment;
  const uint32_t sbtAlignment = rayTracingProperties.shaderGroupBaseAlignment;
  const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
  const uint32_t sbtSize = groupCount * handleSizeAligned;
  const VkBufferUsageFlags sbtBufferUsage =
      VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  const VmaMemoryUsage sbtMemoryUsage = VMA_MEMORY_USAGE_AUTO;

  uint64_t rayGenStride = handleSize;
  uint64_t hitStride = handleSize;
  uint64_t missStride = handleSize;
  auto rayGenSBTInfo = VkBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = rayGenStride,
      .usage = sbtBufferUsage,
  };
  auto rayGenSBTAllocInfo = VmaAllocationCreateInfo{
      .usage = sbtMemoryUsage,
  };
  VkBuffer rayGenSBTBuf;
  vmaCreateBufferWithAlignment(allocator, &rayGenSBTInfo, &rayGenSBTAllocInfo, sbtAlignment, &rayGenSBTBuf, &rayGenAlloc, nullptr);
  rayGenSBT = Buffer(device, rayGenSBTBuf);
  rayGenAddr =
      vk::StridedDeviceAddressRegionKHR(device.getBufferAddress(vk::BufferDeviceAddressInfo(*rayGenSBT)), rayGenStride, rayGenStride);

  auto closestHitSBTInfo = VkBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = hitStride,
      .usage = sbtBufferUsage,
  };
  auto closestHitSBTAllocInfo = VmaAllocationCreateInfo{
      .usage = sbtMemoryUsage,
  };
  VkBuffer closestHitSBTBuf;
  vmaCreateBufferWithAlignment(allocator, &closestHitSBTInfo, &closestHitSBTAllocInfo, sbtAlignment, &closestHitSBTBuf, &closestHitAlloc,
                               nullptr);
  closestHitSBT = Buffer(device, closestHitSBTBuf);
  closestHitAddr =
      vk::StridedDeviceAddressRegionKHR(device.getBufferAddress(vk::BufferDeviceAddressInfo(*closestHitSBT)), hitStride, hitStride);

  auto missSBTInfo = VkBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = missStride,
      .usage = sbtBufferUsage,
  };
  auto missSBTAllocInfo = VmaAllocationCreateInfo{
      .usage = sbtMemoryUsage,
  };
  VkBuffer missSBTBuf;
  vmaCreateBufferWithAlignment(allocator, &missSBTInfo, &missSBTAllocInfo, sbtAlignment, &missSBTBuf, &missAlloc, nullptr);
  missSBT = Buffer(device, missSBTBuf);
  missAddr = vk::StridedDeviceAddressRegionKHR(device.getBufferAddress(vk::BufferDeviceAddressInfo(*missSBT)), missStride, missStride);

  std::vector<unsigned char> sbt = pipeline.getRayTracingShaderGroupHandlesKHR<unsigned char>(0, shaderGroups.size(), sbtSize);

  uploadToGPU(rayGenSBT, sbt.data(), rayGenStride);
  uploadToGPU(closestHitSBT, sbt.data() + handleSize, handleSize);
  uploadToGPU(missSBT, sbt.data() + handleSize * 2, handleSize);
}

void GPURenderer::uploadToGPU(Buffer& buffer, void* data, size_t size)
{
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
      std::move(device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 1)).front());
  copyCmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
  copyCmd.copyBuffer(*stagingBuffer, *buffer, vk::BufferCopy(0, 0, size));
  copyCmd.end();
  queue.submit(vk::SubmitInfo({}, {}, *copyCmd, {}));
  device.waitIdle();
}

void GPURenderer::render(Camera cam, RenderParameter param)
{
  for (uint32_t samp = 0; samp < param.numSamples; ++samp)
  {
    semaphores.push_back(device.createSemaphore(vk::SemaphoreCreateInfo()));
    fences.push_back(device.createFence(vk::FenceCreateInfo()));
  }
  // camera
  {
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(GPUCamera),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    VmaAllocationCreateInfo allocInfo = {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VkBuffer camBuf;
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &camBuf, &cameraAllocation, nullptr);
    cameraBuffer = Buffer(device, camBuf);
    GPUCamera gpuCam = {
        .cameraPosition = cam.position,
        .f = cam.f,
        .cameraForward = glm::normalize(cam.target - cam.position),
        .S_O = cam.S_O,
        .fogEmm = glm::vec3(0, 0, 0),
        .ks = 0,
        .A = cam.A,
        .ka = 0,
    };
    uploadToGPU(cameraBuffer, &gpuCam, sizeof(GPUCamera));
  }
  // radiance accumulator
  {
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .extent =
            {
                .width = (uint32_t)param.width,
                .height = (uint32_t)param.height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    VmaAllocationCreateInfo allocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VkImage radianceImg;
    vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &radianceImg, &radianceAllocation, nullptr);
    radianceAccumulator = Image(device, radianceImg);
    radianceView =
        device.createImageView(vk::ImageViewCreateInfo({}, *radianceAccumulator, vk::ImageViewType::e2D, vk::Format::eR32G32B32A32Sfloat));
  }
  // image
  {
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .extent =
            {
                .width = (uint32_t)param.width,
                .height = (uint32_t)param.height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    VmaAllocationCreateInfo allocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VkImage img;
    vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &img, &imageAllocation, nullptr);
    image = Image(device, img);
    radianceView = device.createImageView(vk::ImageViewCreateInfo({}, *image, vk::ImageViewType::e2D, vk::Format::eR32G32B32A32Sfloat));
  }

  DescriptorSet descriptorSet =
      std::move(device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*descriptorPool, *descriptorLayout)).front());
  std::vector<vk::WriteDescriptorSet> writes;
  // have to use lists so the pointers arent invalidated by push
  std::list<vk::DescriptorBufferInfo> buffers;
  std::list<vk::WriteDescriptorSetAccelerationStructureKHR> accel;
  std::list<vk::DescriptorImageInfo> images;
  uint32_t bindingCounter = 0;
  {
    buffers.push_back(vk::DescriptorBufferInfo(*cameraBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  {
    accel.push_back(vk::WriteDescriptorSetAccelerationStructureKHR(*scene->accelerationStructure));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eAccelerationStructureKHR, nullptr,
                                            nullptr, nullptr, &accel.back()));
  }
  {
    images.push_back(vk::DescriptorImageInfo({}, *radianceView, vk::ImageLayout::eGeneral));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageImage, &images.back(),
                                            nullptr, nullptr));
  }
  {
    images.push_back(vk::DescriptorImageInfo({}, *imageView, vk::ImageLayout::eGeneral));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageImage, &images.back(),
                                            nullptr, nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(*scene->modelBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(*scene->materialBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(*scene->positionBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(*scene->texCoordsBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(*scene->normalsBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(*scene->directionalLightBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(*scene->pointLightBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(*scene->indexBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                                            &buffers.back(), nullptr));
  }
  device.updateDescriptorSets(writes, {});

  // allocate a CommandBuffer from the CommandPool
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, param.numSamples);
  cmdBuffers = CommandBuffers(device, commandBufferAllocateInfo);
  for (uint32_t samp = 0; samp < param.numSamples; ++samp)
  {
    auto& cmd = cmdBuffers[samp];
    cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *pipelineLayout, 0, *descriptorSet, {});
    std::vector<SampleParams> sampleParams = {SampleParams{
        .pass = samp,
        .samplesPerPixel = param.numSamples,
        .numDirectionalLights = (uint32_t)scene->directionalLights.size(),
        .numPointLights = (uint32_t)scene->pointLights.size(),
    }};
    cmd.pushConstants<SampleParams>(*pipelineLayout, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 0,
                                    sampleParams);
    cmd.traceRaysKHR(rayGenAddr, closestHitAddr, missAddr, {}, param.width, param.height, 1);
    cmd.end();
    if (samp == 0)
    {
      queue.submit(vk::SubmitInfo({}, {}, *cmd, *semaphores[samp]), *fences[samp]);
    }
    else
    {
      vk::PipelineStageFlags dstWaitMask = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
      queue.submit(vk::SubmitInfo(*semaphores[samp - 1], dstWaitMask, *cmd, *semaphores[samp]), *fences[samp]);
    }
  }
  for (uint32_t samp = 0; samp < param.numSamples; ++samp)
  {
    assert(device.waitForFences(*fences[samp], true, 1000000) == vk::Result::eSuccess);
  }
}