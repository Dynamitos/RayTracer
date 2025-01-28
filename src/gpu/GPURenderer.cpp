#include "GPURenderer.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_raii.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <slang-com-ptr.h>
#include <slang.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

GPURenderer::GPURenderer()
    : instance(nullptr), physicalDevice(nullptr), device(nullptr), queue(nullptr), cmdPool(nullptr), cmdBuffers(nullptr),
      descriptorLayout(nullptr), descriptorSet(nullptr), descriptorPool(nullptr), pipelineLayout(nullptr), rayGen(nullptr),
      closestHit(nullptr), miss(nullptr), pipeline(nullptr), radianceAccumulator(nullptr), radianceAllocation(nullptr), image(nullptr),
      imageAllocation(nullptr)

{
  createDevice();
  createCommands();
  createDescriptors();
  createShaders();
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
  float queuePriority = 0.0f;
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, computeQueueFamily, 1, &queuePriority);
  vk::DeviceCreateInfo deviceCreateInfo({}, deviceQueueCreateInfo);
  device = Device(physicalDevice, deviceCreateInfo);

  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
  allocatorCreateInfo.physicalDevice = *physicalDevice;
  allocatorCreateInfo.device = *device;
  allocatorCreateInfo.instance = *instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  vmaCreateAllocator(&allocatorCreateInfo, &allocator);
}

void GPURenderer::createCommands()
{
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
  descriptorPool = DescriptorPool(device, vk::DescriptorPoolCreateInfo({}, 4, descriptorPoolSizes));

  // create a PipelineLayout using that DescriptorSetLayout
  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, *descriptorLayout);
  pipelineLayout = PipelineLayout(device, pipelineLayoutCreateInfo);
}

using namespace slang;

void GPURenderer::createShaders()
{
  Slang::ComPtr<IGlobalSession> globalSession;
  createGlobalSession(globalSession.writeRef());
  SessionDesc sessionDesc;
  TargetDesc targetDesc;
  targetDesc.format = SLANG_SPIRV;
  targetDesc.profile = globalSession->findProfile("glsl_450");
  sessionDesc.targets = &targetDesc;
  sessionDesc.targetCount = 1;
  const char* searchPaths[] = {"res/shaders/"};
  sessionDesc.searchPaths = searchPaths;
  sessionDesc.searchPathCount = 1;
  Slang::ComPtr<ISession> session;
  globalSession->createSession(sessionDesc, session.writeRef());

  Slang::ComPtr<IBlob> diagnostics;
  IModule* raygenModule = session->loadModule("RayGen", diagnostics.writeRef());
  if (diagnostics)
  {
    std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
  }
  Slang::ComPtr<IEntryPoint> rayGenEntry;
  raygenModule->findEntryPointByName("rayGen", rayGenEntry.writeRef());

  IModule* closestHitModule = session->loadModule("ClosestHit", diagnostics.writeRef());
  if (diagnostics)
  {
    std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
  }
  Slang::ComPtr<IEntryPoint> closestHitEntry;
  closestHitModule->findEntryPointByName("closestHit", closestHitEntry.writeRef());

  IComponentType* components[] = {raygenModule, rayGenEntry, closestHitModule, closestHitEntry};
  Slang::ComPtr<IComponentType> program;
  session->createCompositeComponentType(components, 4, program.writeRef());

  Slang::ComPtr<IComponentType> linkedProgram;
  program->link(linkedProgram.writeRef(), diagnostics.writeRef());

  Slang::ComPtr<IBlob> rayGenCode;
  linkedProgram->getEntryPointCode(0, 0, rayGenCode.writeRef(), diagnostics.writeRef());

  Slang::ComPtr<IBlob> closestHitCode;
  linkedProgram->getEntryPointCode(1, 0, closestHitCode.writeRef(), diagnostics.writeRef());

  rayGen =
      ShaderModule(device, vk::ShaderModuleCreateInfo({}, rayGenCode->getBufferSize(), (const uint32_t*)rayGenCode->getBufferPointer()));

  closestHit = ShaderModule(
      device, vk::ShaderModuleCreateInfo({}, closestHitCode->getBufferSize(), (const uint32_t*)closestHitCode->getBufferPointer()));

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

  {
    shaderStages.push_back(VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .module = *rayGen,
        .pName = "rayGen",
        .pSpecializationInfo = nullptr,
    });
    shaderGroups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .pNext = nullptr,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader = static_cast<uint32_t>(shaderStages.size() - 1),
        .closestHitShader = VK_SHADER_UNUSED_KHR,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .pShaderGroupCaptureReplayHandle = nullptr,
    });
  }
  {
    shaderStages.push_back(VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        .module = *closestHit,
        .pName = "closestHit",
        .pSpecializationInfo = nullptr,
    });
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
    shaderGroups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .pNext = nullptr,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        .generalShader = VK_SHADER_UNUSED_KHR,
        .closestHitShader = hitIndex,
        .anyHitShader = anyHitIndex,
        .intersectionShader = intersectionIndex,
        .pShaderGroupCaptureReplayHandle = nullptr,
    });
  }
}

void GPURenderer::render(Camera cam, RenderParameter param)
{
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

    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &cameraBuffer, &cameraAllocation, nullptr);
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
    vmaCopyMemoryToAllocation(allocator, &gpuCam, cameraAllocation, 0, sizeof(GPUCamera));
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
    radianceView = device.createImageView(vk::ImageViewCreateInfo({}, *radianceAccumulator, vk::ImageViewType::e2D, vk::Format::eR32G32B32A32Sfloat));
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

  DescriptorSet descriptorSet = std::move(device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(*descriptorPool, *descriptorLayout)).front());
  std::vector<vk::WriteDescriptorSet> writes;
  // have to use lists so the pointers arent invalidated by push
  std::list<vk::DescriptorBufferInfo> buffers;
  std::list<vk::WriteDescriptorSetAccelerationStructureKHR> accel;
  std::list<vk::DescriptorImageInfo> images;
  uint32_t bindingCounter = 0;
  {
    buffers.push_back(vk::DescriptorBufferInfo(cameraBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &buffers.back(), nullptr));
  }
  {
    accel.push_back(vk::WriteDescriptorSetAccelerationStructureKHR(1, scene->accelerationStructure));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eAccelerationStructureKHR, nullptr, nullptr, nullptr, &accel.back()));
  }
  {
    images.push_back(vk::DescriptorImageInfo({}, radianceView, vk::ImageLayout::eGeneral));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageImage, &images.back(), nullptr, nullptr));
  }
  {
    images.push_back(vk::DescriptorImageInfo({}, imageView, vk::ImageLayout::eGeneral));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageImage, &images.back(), nullptr, nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(scene->modelBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(scene->materialBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(scene->positionsBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(scene->texCoordsBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(scene->normalsBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(scene->directionalLightsBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(scene->pointLightsBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffers.back(), nullptr));
  }
  {
    buffers.push_back(vk::DescriptorBufferInfo(scene->indexBuffer));
    writes.push_back(vk::WriteDescriptorSet(*descriptorSet, bindingCounter++, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffers.back(), nullptr));
  }
  device.updateDescriptorSets(writes, {});
  
  semaphores.resize(param.numSamples);
  fences.resize(param.numSamples);
  for (uint32_t samp = 0; samp < param.numSamples; ++samp)
  {
    semaphores[samp] = device.createSemaphore(vk::SemaphoreCreateInfo());
    fences[samp] = device.createFence(vk::FenceCreateInfo());
  }
  // allocate a CommandBuffer from the CommandPool
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, param.numSamples);
  cmdBuffers = CommandBuffers(device, commandBufferAllocateInfo);
  for (uint32_t samp = 0; samp < param.numSamples; ++samp)
  {
    auto& cmd = cmdBuffers[samp];
    cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelineLayout, 0, descriptorSet, {});
    cmd.traceRays(param.width, param.height, 1);
    cmd.end();
    if (samp == 0)
    {
      queue.submit(vk::SubmitInfo({}, cmd, semaphores[samp]), fences[samp]);
    }
    else
    {
      queue.submit(vk::SubmitInfo(semaphores[samp-1], vk::PipelineStageFlagBits::eRayTracingShaderKHR, cmd, semaphores[samp]), fences[samp]);
    }
  }
  for (uint32_t samp = 0; samp < param.numSamples; ++samp)
  {
    device.waitForFences(fences[samp], true, 1000000);
  }
}