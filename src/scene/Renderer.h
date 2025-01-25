#pragma once
#include "Scene.h"
#include "window/Window.h"
#include "util/Camera.h"
#include "ThreadPool.h"

struct RenderParameter
{
    int width;
    int height;
    int numSamples;
};

class Renderer
{
  public:
    Renderer();
    virtual ~Renderer();
    void startRender(Camera cam, RenderParameter params);
    constexpr const std::vector<glm::vec3>& getImage() const { return image; }
  private:
    virtual void render(Camera cam, RenderParameter params);
    ThreadPool threadPool;
    std::thread worker;
    std::atomic_bool pendingCancel = false;
    // the thing being displayed
    std::vector<glm::vec3> image;
    // radiance accumulator
    std::vector<glm::vec3> accumulator;
    std::vector<PointLight> pointLights;
    std::vector<DirectionalLight> directionalLights;
    Scene bvh;
};