#pragma once
#include "BVH.h"
#include "window/Window.h"

class Scene
{
  public:
    Scene();
    ~Scene();
    void render();
  private:
    Window window;
    std::vector<unsigned char> image;
    BVH bvh;
};