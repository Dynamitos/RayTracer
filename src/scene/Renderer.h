#pragma once
#include "Scene.h"
#include "util/Camera.h"
#include <thread>
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
  virtual void addPointLight(PointLight point) = 0;
  virtual void addDirectionalLight(DirectionalLight dir) = 0;
  virtual void addModel(PModel model, glm::mat4 transform) = 0;
  virtual void addModels(std::vector<PModel> models, glm::mat4 transform) = 0;
  virtual void generate() = 0;
  void startRender(Camera cam, RenderParameter params);
  constexpr const std::vector<float>& getSampleTimes() const { return sampleTimes; }
  constexpr const float getLastSampleTime() const { return sampleTimes.empty() ? 0 : sampleTimes.back(); }
  constexpr const float getAverageSampleTime() const
  {
    return std::accumulate(sampleTimes.begin(), sampleTimes.end(), 0.0f) / sampleTimes.size();
  }
  // main thread
  virtual void beginFrame() = 0;
  virtual void update() = 0;

protected:
  virtual void render(Camera cam, RenderParameter params) = 0;
  std::thread worker;
  std::atomic_bool running = false;
  std::vector<float> sampleTimes;
  float lastSampleTime;
  float averageSampleTime;
};