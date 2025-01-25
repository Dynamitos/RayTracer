#include "Renderer.h"

Renderer::Renderer()
    : instance(nullptr), physicalDevice(nullptr), device(nullptr), queue(nullptr), cmdPool(nullptr), cmdBuffers(nullptr),
      descriptorLayout(nullptr), descriptorSet(nullptr), descriptorPool(nullptr), pipelineLayout(nullptr), rayGen(nullptr),
      closestHit(nullptr), miss(nullptr), pipeline(nullptr)

{
  

  vk::RayTracingPipelineCreateInfoKHR pipelineCreateInfo(0, );
}

Renderer::~Renderer() {}

void Renderer::createDevice() {
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
}

void Renderer::createCommands()
{
  vk::CommandPoolCreateInfo commandPoolCreateInfo({}, computeQueueFamily);
  cmdPool = CommandPool(device, commandPoolCreateInfo);

  // allocate a CommandBuffer from the CommandPool
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 10);
  cmdBuffers = vk::raii::CommandBuffers(device, commandBufferAllocateInfo);
}

void Renderer::createDescriptors() {
  vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
  vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, descriptorSetLayoutBinding);
  descriptorLayout = DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);

  // create a PipelineLayout using that DescriptorSetLayout
  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, *descriptorLayout);
  pipelineLayout = PipelineLayout(device, pipelineLayoutCreateInfo);
}

void Renderer::createShaders() {
    
}

void Renderer::render(Camera cam, RenderParameter param) {}