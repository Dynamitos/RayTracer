#pragma once
#include "BVH.h"
#include "window/Window.h"
#include "util/Camera.h"
#include "ThreadPool.h"

struct RenderParameter
{
    int width;
    int height;
    int numSamples;
};

struct PointLight
{
  glm::vec3 position;
  glm::vec3 color;
  float attenuation;
};

struct DirectionalLight
{
  glm::vec3 direction;
  glm::vec3 color;
};

class Scene
{
  public:
    Scene();
    virtual ~Scene();
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
    BVH bvh;
};