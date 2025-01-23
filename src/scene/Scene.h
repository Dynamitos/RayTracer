#pragma once
#include "BVH.h"
#include "window/Window.h"

class Scene
{
  public:
    void render();
  private:
    Window window;
    std::vector<uint32_t> image;
    BVH bvh;
};