#pragma once
#include "scene/Scene.h"
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

class MetalScene : public Scene
{
public:
    MetalScene(MTL::Device* device, MTL::CommandQueue* queue);
    virtual ~MetalScene();

    virtual void createRayTracingHierarchy() override;

    MTL::Device* device;
    MTL::CommandQueue* queue;

    MTL::Buffer* indicesBuffer;
    MTL::Buffer* positionBuffer;
    MTL::Buffer* texCoordsBuffer;
    MTL::Buffer* normalBuffer;
    MTL::Buffer* modelRefsBuffer;
    MTL::Buffer* directionalLightBuffer;
    MTL::Buffer* pointLightBuffer;
    MTL::Buffer* instanceBuffer;

    MTL::AccelerationStructure* accelerationStructure;
};