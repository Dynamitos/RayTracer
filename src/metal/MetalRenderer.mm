#include "MetalRenderer.h"
#include "metal/MetalScene.h"
#include "scene/Renderer.h"
#include "util/Camera.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_metal.h>

  NSWindow* window;
CAMetalLayer* metalLayer;
id<CAMetalDrawable> drawable;

id<MTLDevice> device;
id<MTLLibrary> library;
id<MTLCommandQueue> queue;
id<MTLFunction> function;
id<MTLComputePipelineState> computePipeline;
id<MTLTexture> accumulator;
id<MTLTexture> resultTexture;
  
MTLRenderPassDescriptor* renderPass;
id<MTLRenderCommandEncoder> renderEncoder;
id<MTLCommandBuffer> renderCmd;

MetalRenderer::MetalRenderer()
{
    IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO();
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  width = 1920;
  height = 1080;
  device = MTLCreateSystemDefaultDevice();

  library = [device newDefaultLibrary];

  queue = [device newCommandQueue];

  scene = new MetalScene(device, queue);

    function = [library newFunctionWithName:@"computeKernel"];

  NSError* error;
    computePipeline = [device newComputePipelineStateWithFunction:function error:&error];

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

  metalLayer = [CAMetalLayer layer];
  metalLayer.device = device;
  metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  metalLayer.drawableSize = CGSizeMake(w, h);
  metalLayer.framebufferOnly = true;
  NSWindow* cocoaWindow = glfwGetCocoaWindow(handle);
    [[cocoaWindow contentView] setLayer:metalLayer];
  [[cocoaWindow contentView] setWantsLayer:YES];
  [[cocoaWindow contentView] setNeedsLayout:YES];
 
    renderPass = [[MTLRenderPassDescriptor alloc] init];
}

MetalRenderer::~MetalRenderer() {}

void MetalRenderer::addPointLight(PointLight point) { scene->addPointLight(point); }
void MetalRenderer::addDirectionalLight(DirectionalLight dir) { scene->addDirectionalLight(dir); }
void MetalRenderer::addModel(PModel model, glm::mat4 transform) { scene->addModel(std::move(model), transform); }
void MetalRenderer::addModels(std::vector<PModel> models, glm::mat4 transform) { scene->addModels(std::move(models), transform); }
void MetalRenderer::generate() { scene->generate(); }

void MetalRenderer::beginFrame()
{
  drawable = [metalLayer nextDrawable];
  renderCmd = [queue commandBuffer];
    renderPass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    renderPass.colorAttachments[0].texture = drawable.texture;
    renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderEncoder = [renderCmd renderCommandEncoderWithDescriptor:renderPass];
    ImGui_ImplMetal_NewFrame(renderPass);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void MetalRenderer::update()
{
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), renderCmd, renderEncoder);
    [renderEncoder endEncoding];
    [renderCmd presentDrawable:drawable];
    [renderCmd commit];
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

    MTLTextureDescriptor* texDescriptor = [[MTLTextureDescriptor alloc] init];
    [texDescriptor setWidth:parameter.width];
    [texDescriptor setHeight:parameter.height];
    [texDescriptor setPixelFormat:MTLPixelFormatRGBA32Float];
    [texDescriptor setUsage:MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead];
    accumulator = [device newTextureWithDescriptor:texDescriptor];
    resultTexture = [device newTextureWithDescriptor:texDescriptor];
  for (uint i = 0; i < parameter.numSamples; ++i)
  {
    id<MTLCommandBuffer> cmdBuffer = [queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [cmdBuffer computeCommandEncoder];
    // cmdBuffer->addCompletedHandler([this](MTL::CommandBuffer* cmdBuffer)
    //                                { std::memcpy(image.data(), resultTexture->buffer(), image.size() * sizeof(glm::vec3)); });
    
      SampleParams sample = {
          .pass = i,
              .samplesPerPixel = parameter.numSamples,
          .numDirectionalLights = scene->getNumDirLights(),
          .numPointLights = scene->getNumPointLights(),
      };
    [encoder setComputePipelineState:computePipeline];
    [encoder setBuffer:scene->indicesBuffer offset:0 atIndex:0];
    [encoder setBuffer:scene->positionBuffer offset:0 atIndex:1];
    [encoder setBuffer:scene->texCoordsBuffer offset:0 atIndex:2];
    [encoder setBuffer:scene->normalBuffer offset:0 atIndex:3];
    [encoder setBuffer:scene->modelRefsBuffer offset:0 atIndex:4];
    [encoder setBuffer:scene->directionalLightBuffer offset:0 atIndex:5];
    [encoder setBuffer:scene->pointLightBuffer offset:0 atIndex:6];
    [encoder setBuffer:scene->instanceBuffer offset:0 atIndex:7];
    [encoder setAccelerationStructure:scene->accelerationStructure atBufferIndex:8];
    [encoder setTexture:accumulator atIndex:0];
    [encoder setTexture:resultTexture atIndex:1];
    [encoder setBytes:&gpuCam length:sizeof(GPUCamera) atIndex:9];
    [encoder setBytes:&sample length:sizeof(SampleParams) atIndex:10];
    [encoder useResource:scene->instanceBuffer usage:MTLResourceUsageRead];
    [encoder useResource:scene->positionBuffer usage:MTLResourceUsageRead];
    [encoder useResource:scene->texCoordsBuffer usage:MTLResourceUsageRead];
    [encoder useResource:scene->normalBuffer usage:MTLResourceUsageRead];
    [encoder useResource:scene->modelRefsBuffer usage:MTLResourceUsageRead];
    if (scene->getNumDirLights() > 0)
    {
      [encoder useResource:scene->directionalLightBuffer usage:MTLResourceUsageRead];
    }
    if (scene->getNumPointLights() > 0)
    {
        [encoder useResource:scene->pointLightBuffer usage:MTLResourceUsageRead];
    }
    [encoder useResource:scene->instanceBuffer usage:MTLResourceUsageRead];
    [encoder useResource:scene->accelerationStructure usage:MTLResourceUsageRead];
    [encoder useResource:accumulator usage:MTLResourceUsageWrite];
    [encoder useResource:resultTexture usage:MTLResourceUsageWrite];
    NSUInteger width = (NSUInteger)parameter.width;
    NSUInteger height = (NSUInteger)parameter.height;
    MTLSize threadsPerThreadgroup = MTLSizeMake(8, 8, 1);
    MTLSize threadgroups = MTLSizeMake((width + threadsPerThreadgroup.width - 1) / threadsPerThreadgroup.width,
                                       (height + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height, 1);
    [encoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
    [encoder endEncoding];
    [cmdBuffer commit];
    [cmdBuffer waitUntilCompleted];
  }
}
