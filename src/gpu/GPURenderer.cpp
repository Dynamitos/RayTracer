#include "GPURenderer.h"
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

  VmaAllocator allocator;
  vmaCreateAllocator(&allocatorCreateInfo, &allocator);
}

void GPURenderer::createCommands()
{
  vk::CommandPoolCreateInfo commandPoolCreateInfo({}, computeQueueFamily);
  cmdPool = CommandPool(device, commandPoolCreateInfo);

  // allocate a CommandBuffer from the CommandPool
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 10);
  cmdBuffers = vk::raii::CommandBuffers(device, commandBufferAllocateInfo);
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

  // create a PipelineLayout using that DescriptorSetLayout
  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, *descriptorLayout);
  pipelineLayout = PipelineLayout(device, pipelineLayoutCreateInfo);
}

using namespace slang;

void GPURenderer::createShaders()
{
  /*
  Slang::ComPtr<IGlobalSession> globalSession;
  SlangGlobalSessionDesc desc = {};
  createGlobalSession(&desc, globalSession.writeRef());
  SessionDesc sessionDesc;
  TargetDesc targetDesc;
  targetDesc.format = SLANG_SPIRV;
  targetDesc.profile = globalSession->findProfile("glsl_450");
  sessionDesc.targets = &targetDesc;
  sessionDesc.targetCount = 1;
  const char* searchPaths[] = {"res/shaders/"};
  sessionDesc.searchPaths = searchPaths;
  sessionDesc.searchPathCount = 1;
  /* ... fill in `sessionDesc` ...
  Slang::ComPtr<ISession> session;
  globalSession->createSession(sessionDesc, session.writeRef());

  Slang::ComPtr<IBlob> diagnostics;
  IModule* module = session->loadModule("MyShaders", diagnostics.writeRef());
  if (diagnostics)
  {
    std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
  }
  Slang::ComPtr<IEntryPoint> computeEntryPoint;
  module->findEntryPointByName("myComputeMain", computeEntryPoint.writeRef());
  IComponentType* components[] = {module, computeEntryPoint};
  Slang::ComPtr<IComponentType> program;
  session->createCompositeComponentType(components, 2, program.writeRef());
  Slang::ComPtr<IComponentType> linkedProgram;
  Slang::ComPtr<ISlangBlob> diagnosticBlob;
  program->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
  int entryPointIndex = 0; // only one entry point
  int targetIndex = 0;     // only one target
  Slang::ComPtr<IBlob> kernelBlob;
  linkedProgram->getEntryPointCode(entryPointIndex, targetIndex, kernelBlob.writeRef(), diagnostics.writeRef());
  */
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
    vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &radianceAccumulator, &radianceAllocation, nullptr);
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
    vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &image, &imageAllocation, nullptr);
  }
}