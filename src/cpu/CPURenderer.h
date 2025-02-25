#pragma once
#include "cpu/CPUScene.h"
#include "scene/Renderer.h"
#include "util/Camera.h"

class CPURenderer : public Renderer
{
public:
    virtual void render(Camera camera, RenderParameter params) override;
private:
    CPUScene* scene;
    ThreadPool threadPool;
    // radiance accumulator
    std::vector<glm::vec3> accumulator;
};