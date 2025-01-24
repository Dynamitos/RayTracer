#pragma once
#include "BVH.h"
#include "window/Window.h"
#include "util/Camera.h"

class Scene
{
  public:
    Scene();
    ~Scene();
    void render(Camera cam, int width, int height, int numSamples);
  private:
    // the thing being displayed
    std::vector<unsigned char> image;
    // radiance accumulator
    std::vector<unsigned char> accumulator;
    BVH bvh;
};