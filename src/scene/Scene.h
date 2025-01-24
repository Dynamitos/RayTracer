#pragma once
#include "BVH.h"
#include "window/Window.h"
#include "util/Camera.h"

/// <summary>
/// Ray1
/// Ray2
/// Ray3
/// Ray4
/// Ray5
/// GammaResolve
/// Ray1
/// Ray2
/// Ray3
/// </summary>
class Scene
{
  public:
    Scene();
    ~Scene();
    void render(Camera cam, int width, int height, int numSamples);
    constexpr const std::vector<unsigned char>& getImage() const { return image; }
  private:
    // the thing being displayed
    std::vector<unsigned char> image;
    // radiance accumulator
    std::vector<unsigned char> accumulator;
    BVH bvh;
};