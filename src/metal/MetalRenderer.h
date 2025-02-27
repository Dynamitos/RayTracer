#pragma once
#include "metal/MetalScene.h"
#include "scene/Renderer.h"
#include "util/Camera.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/QuartzCore.hpp>

struct GPUCamera
{
  glm::vec3 cameraPosition;
  float f;
  glm::vec3 cameraForward;
  float S_O;
  glm::vec3 fogEmm;
  float ks;
  float A;
  float ka;
  glm::vec2 sensorSize;
  uint width;
  uint height;
};

struct SampleParams
{
  uint pass;
  uint samplesPerPixel;
  uint numDirectionalLights;
  uint numPointLights;
};

class MetalRenderer : public Renderer
{
public:
  MetalRenderer();
  virtual ~MetalRenderer();
  virtual void addPointLight(PointLight point) override { scene->addPointLight(point); }
  virtual void addDirectionalLight(DirectionalLight dir) override { scene->addDirectionalLight(dir); }
  virtual void addModel(PModel model, glm::mat4 transform) override { scene->addModel(std::move(model), transform); }
  virtual void addModels(std::vector<PModel> models, glm::mat4 transform) override { scene->addModels(std::move(models), transform); }
  virtual void generate() override { scene->generate(); }
  virtual void render(Camera camera, RenderParameter params) override;

  virtual void beginFrame() override;
  virtual void update() override;

private:
  uint width;
  uint height;
  uint framebufferWidth;
  uint framebufferHeight;
  GLFWwindow* handle;
  CA::MetalLayer* metalLayer;
  CA::MetalDrawable* drawable;

  MTL::Device* device;
  MTL::Library* library;
  MTL::CommandQueue* queue;
  MTL::Function* function;
  MTL::ComputePipelineState* computePipeline;
  MTL::Texture* accumulator;
  MTL::Texture* resultTexture;
    
    MTL::RenderPassDescriptor* renderPass;
    MTL::RenderCommandEncoder* renderEncoder;
    MTL::CommandBuffer* renderCmd;

  MetalScene* scene;
};

