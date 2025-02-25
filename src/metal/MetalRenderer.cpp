#include "MetalRenderer.h"
#include "metal/MetalScene.h"
#include "scene/Renderer.h"
#include "util/Camera.h"
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

MetalRenderer::MetalRenderer()
{
  device = MTL::CreateSystemDefaultDevice();

  library = device->newDefaultLibrary();

  queue = device->newCommandQueue();

  scene = new MetalScene(device, queue);

  function = library->newFunction(NS::String::string("computeKernel", NS::ASCIIStringEncoding));

  NS::Error* error;
  computePipeline = device->newComputePipelineState(function, &error);
}

MetalRenderer::~MetalRenderer() {}

void MetalRenderer::render(Camera camera, RenderParameter parameter)
{
    MTL::Buffer* cameraBuffer = device->newBuffer(sizeof(GPUCamera), 0);
    *(GPUCamera*)cameraBuffer->contents() = GPUCamera {
      .cameraPosition = camera.position,
      .A = camera.A,
      .cameraForward = camera.target - camera.position,
      .f = camera.f,
      .S_O = camera.S_O,
      .sensorSize = camera.sensorSize,
      .width = parameter.width,
      .height = parameter.height,
  };
    
  SampleParams sample = {
      .samplesPerPixel = parameter.numSamples,
      .numDirectionalLights = scene->getNumDirLights(),
      .numPointLights = scene->getNumPointLights(),
  };
    accumulator = device->newBuffer(parameter.width * parameter.height * sizeof(glm::vec3), 0);
    resultTexture = device->newBuffer(parameter.width * parameter.height * sizeof(glm::vec3), 0);
  for (uint i = 0; i < parameter.numSamples; ++i)
  {
    MTL::CommandBuffer* cmdBuffer = queue->commandBuffer();
    MTL::ComputeCommandEncoder* encoder = cmdBuffer->computeCommandEncoder();
    //cmdBuffer->addCompletedHandler([this](MTL::CommandBuffer* cmdBuffer)
    //                               { std::memcpy(image.data(), resultTexture->buffer(), image.size() * sizeof(glm::vec3)); });
      sample.pass = i;
    MTL::Buffer* sampleBuffer = device->newBuffer(sizeof(SampleParams), 0);
      *(SampleParams*)sampleBuffer->contents() = sample;
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
    encoder->setBuffer(accumulator, 0, 9);
    encoder->setBuffer(resultTexture, 0, 10);
      encoder->setBuffer(cameraBuffer, 0, 11);
      encoder->setBuffer(sampleBuffer, 0, 12);
    encoder->useResource(scene->instanceBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->positionBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->texCoordsBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->normalBuffer, MTL::ResourceUsageRead);
    encoder->useResource(scene->modelRefsBuffer, MTL::ResourceUsageRead);
      if(scene->getNumDirLights() > 0)
      {
          encoder->useResource(scene->directionalLightBuffer, MTL::ResourceUsageRead);}
      if(scene->getNumPointLights() > 0)
      {encoder->useResource(scene->pointLightBuffer, MTL::ResourceUsageRead);}
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
    cmdBuffer->commit();
    cmdBuffer->waitUntilCompleted();
      std::memcpy(image.data(), resultTexture->contents(), image.size() * sizeof(glm::vec3));
  }
}
