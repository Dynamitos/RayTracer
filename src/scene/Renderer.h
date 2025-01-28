#pragma once
#include "Scene.h"
#include "ThreadPool.h"
#include "util/Camera.h"
#include "window/Window.h"
#include <numeric>

struct RenderParameter
{
  uint32_t width;
  uint32_t height;
  uint32_t numSamples;
};

class Renderer
{
public:
  Renderer();
  virtual ~Renderer();
  void startRender(Camera cam, RenderParameter params);
  constexpr const std::vector<glm::vec3>& getImage() const { return image; }
  constexpr const std::vector<float>& getSampleTimes() const { return sampleTimes; }
  constexpr const float getLastSampleTime() const { return sampleTimes.empty() ? 0 : sampleTimes.back(); }
  constexpr const float getAverageSampleTime() const
  {
    return std::accumulate(sampleTimes.begin(), sampleTimes.end(), 0.0f) / sampleTimes.size();
  }

protected:
  virtual void render(Camera cam, RenderParameter params);
  ThreadPool threadPool;
  std::thread worker;
  std::atomic_bool running = false;
  std::vector<float> sampleTimes;
  float lastSampleTime;
  float averageSampleTime;
  // the thing being displayed
  std::vector<glm::vec3> image;
  // radiance accumulator
  std::vector<glm::vec3> accumulator;
  std::vector<PointLight> pointLights;
  std::vector<DirectionalLight> directionalLights;
  std::unique_ptr<Scene> scene;
};