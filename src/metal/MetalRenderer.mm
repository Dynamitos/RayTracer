#include "MetalRenderer.h"
#include "Foundation/NSSharedPtr.hpp"
#include "Metal/MTLBlitCommandEncoder.hpp"
#include "Metal/MTLDrawable.hpp"
#include "Metal/MTLRenderPass.hpp"
#include "QuartzCore/CAMetalLayer.hpp"
#include "metal/MetalScene.h"
#include "scene/Renderer.h"
#include "util/Camera.h"
#include <Foundation/Foundation.hpp>
#include <GLFW/glfw3.h>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <QuartzCore/CAMetalLayer.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_metal.h>

MetalRenderer::MetalRenderer()
{
    IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO();
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  width = 1920;
  height = 1080;
  device = MTL::CreateSystemDefaultDevice();

  library = device->newDefaultLibrary();

  queue = device->newCommandQueue();

  scene = new MetalScene(device, queue);

  function = library->newFunction(NS::String::string("computeKernel", NS::ASCIIStringEncoding));

  NS::Error* error;
  computePipeline = device->newComputePipelineState(function, &error);

    glfwInit();
  float xscale = 1, yscale = 1;
  glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  handle = glfwCreateWindow(width / xscale, height / yscale, "RayTracer", nullptr, nullptr);

  int w, h;
  glfwGetFramebufferSize(handle, &w, &h);
  framebufferWidth = width;
  framebufferHeight = height;

  ImGui_ImplGlfw_InitForOpenGL(handle, false);
  ImGui_ImplMetal_Init((__bridge id)device);

  metalLayer = CA::MetalLayer::layer();
  metalLayer->setDevice(device);
  metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  metalLayer->setDrawableSize(CGSizeMake(w, h));
  metalLayer->setFramebufferOnly(true);
  CAMetalLayer* native_layer = (__bridge CAMetalLayer*)metalLayer;
  NSWindow* cocoaWindow = glfwGetCocoaWindow(handle);
  [[cocoaWindow contentView] setLayer:native_layer];
  [[cocoaWindow contentView] setWantsLayer:YES];
  [[cocoaWindow contentView] setNeedsLayout:YES];
 
    renderPass= MTL::RenderPassDescriptor::alloc()->init();
}

MetalRenderer::~MetalRenderer() {}

void MetalRenderer::beginFrame()
{
  drawable = metalLayer->nextDrawable();
  renderCmd = queue->commandBuffer();
  MTL::RenderPassColorAttachmentDescriptor* colorAttachment = MTL::RenderPassColorAttachmentDescriptor::alloc()->init();
  colorAttachment->setClearColor(MTL::ClearColor(0, 0, 0, 0));
    colorAttachment->setTexture(drawable->texture());
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setStoreAction(MTL::StoreActionStore);
  renderPass->colorAttachments()->setObject(colorAttachment, 0);
    renderEncoder = renderCmd->renderCommandEncoder(renderPass);
    ImGui_ImplMetal_NewFrame((__bridge id)renderPass);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void MetalRenderer::update()
{
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), (__bridge id)renderCmd, (__bridge id)renderEncoder);
    renderCmd->presentDrawable((const MTL::Drawable*)drawable);
    renderCmd->commit();
}

void MetalRenderer::render(Camera camera, RenderParameter parameter)
{
  GPUCamera gpuCam = {
      .cameraPosition = camera.position,
      .A = camera.A,
      .cameraForward = camera.target - camera.position,
      .f = camera.f,
      .S_O = camera.S_O,
      .sensorSize = camera.sensorSize,
      .width = parameter.width,
      .height = parameter.height,
  };

    MTL::TextureDescriptor* texDescriptor = MTL::TextureDescriptor::alloc()->init();
    texDescriptor->setWidth(parameter.width);
    texDescriptor->setHeight(parameter.height);
    texDescriptor->setPixelFormat(MTL::PixelFormatRGBA32Float);
    texDescriptor->setUsage(MTL::TextureUsageShaderWrite | MTL::TextureUsageShaderRead);
    accumulator = device->newTexture(texDescriptor);
    resultTexture = device->newTexture(texDescriptor);
  for (uint i = 0; i < parameter.numSamples; ++i)
  {
    MTL::CommandBuffer* cmdBuffer = queue->commandBuffer();
    MTL::ComputeCommandEncoder* encoder = cmdBuffer->computeCommandEncoder();
    // cmdBuffer->addCompletedHandler([this](MTL::CommandBuffer* cmdBuffer)
    //                                { std::memcpy(image.data(), resultTexture->buffer(), image.size() * sizeof(glm::vec3)); });
    
      SampleParams sample = {
          .pass = i,
              .samplesPerPixel = parameter.numSamples,
          .numDirectionalLights = scene->getNumDirLights(),
          .numPointLights = scene->getNumPointLights(),
      };
    encoder->setComputePipelineState(computePipeline);
    encoder->setBuffer(scene->indicesBuffer, 0, 0);
    encoder->setBuffer(scene->positionBuffer, 0, 1);
    encoder->setBuffer(scene->texCoordsBuffer, 0, 2);
    encoder->setBuffer(scene->normalBuffer, 0, 3);
    encoder->setBuffer(scene->modelRefsBuffer, 0, 4);
    encoder->setBuffer(scene->directionalLightBuffer, 0, 5);
    encoder->setBuffer(scene->pointLightBuffer, 0, 6);
    encoder->setBuffer(scene->instanceBuffer, 0, 7);
    encoder->setAccelerationStructure(scene->accelerationStructure, 8);
    encoder->setTexture(accumulator, 0);
    encoder->setTexture(resultTexture, 1);
      encoder->setBytes(&gpuCam, sizeof(GPUCamera), 9);
      encoder->setBytes(&sample, sizeof(SampleParams), 10);
    encoder->useResource(scene->instanceBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->positionBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->texCoordsBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->normalBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->modelRefsBuffer, MTL::ResourceUsageRead);
    if (scene->getNumDirLights() > 0)
    {
      encoder->useResource(scene->directionalLightBuffer, MTL::ResourceUsageRead);
    }
    if (scene->getNumPointLights() > 0)
    {
      encoder->useResource(scene->pointLightBuffer, MTL::ResourceUsageRead);
    }
    encoder->useResource(scene->instanceBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->accelerationStructure, MTL::ResourceUsageRead);
    encoder->useResource(accumulator, MTL::ResourceUsageWrite);
    encoder->useResource(resultTexture, MTL::ResourceUsageWrite);
    NS::UInteger width = (NS::UInteger)parameter.width;
    NS::UInteger height = (NS::UInteger)parameter.height;
    MTL::Size threadsPerThreadgroup = MTL::Size(8, 8, 1);
    MTL::Size threadgroups = MTL::Size((width + threadsPerThreadgroup.width - 1) / threadsPerThreadgroup.width,
                                       (height + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height, 1);
    encoder->dispatchThreadgroups(threadgroups, threadsPerThreadgroup);
    encoder->endEncoding();
    MTL::BlitCommandEncoder* blitCommandEncoder = cmdBuffer->blitCommandEncoder();
      blitCommandEncoder->copyFromTexture(resultTexture, 0, 0, MTL::Origin(), MTL::Size(parameter.width, parameter.height, 1), drawable->texture(), 0, 0, MTL::Origin());
    cmdBuffer->commit();
    cmdBuffer->waitUntilCompleted();
  }
}
