#pragma once
#include "scene/Scene.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

using namespace vk::raii;

struct Renderer : public Scene
{
public:
  Renderer();
  virtual ~Renderer();

private:
  Context context;
  Instance instance;
  PhysicalDevice physicalDevice;
  Device device;
  Queue queue;

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

  virtual void render(Camera cam, RenderParameter param);
};