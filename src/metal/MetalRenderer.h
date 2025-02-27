#pragma once
#include "scene/Renderer.h"
#include "util/Camera.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

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
    virtual void addPointLight(PointLight point) override;
    virtual void addDirectionalLight(DirectionalLight dir) override;
    virtual void addModel(PModel model, glm::mat4 transform) override;
    virtual void addModels(std::vector<PModel> models, glm::mat4 transform) override;
    virtual void generate() override;
  virtual void render(Camera camera, RenderParameter params) override;

  virtual void beginFrame() override;
  virtual void update() override;

private:
  uint width;
  uint height;
  uint framebufferWidth;
  uint framebufferHeight;
  GLFWwindow* handle;

  class MetalScene* scene;
};

