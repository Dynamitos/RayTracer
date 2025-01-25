#include "GPURenderer.h"
#include <slang-com-ptr.h>
#include <slang.h>

GPURenderer::GPURenderer()
    : instance(nullptr), physicalDevice(nullptr), device(nullptr), queue(nullptr), cmdPool(nullptr), cmdBuffers(nullptr),
      descriptorLayout(nullptr), descriptorSet(nullptr), descriptorPool(nullptr), pipelineLayout(nullptr), rayGen(nullptr),
      closestHit(nullptr), miss(nullptr), pipeline(nullptr)

{
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
  vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
  vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, descriptorSetLayoutBinding);
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

void GPURenderer::render(Camera cam, RenderParameter param) {}