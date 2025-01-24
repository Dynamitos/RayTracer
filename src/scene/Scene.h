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

class Scene
{
  public:
    Scene();
    ~Scene();
    void render(Camera cam, RenderParameter params);
    constexpr const std::vector<unsigned char>& getImage() const { return image; }
  private:
    std::atomic_bool pendingCancel = false;
    ThreadPool threadPool;
    std::thread worker;
    // the thing being displayed
    std::vector<unsigned char> image;
    // radiance accumulator
    std::vector<unsigned char> accumulator;
    BVH bvh;
};