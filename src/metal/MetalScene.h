#pragma once
#include "scene/Scene.h"
#include <Metal/Metal.h>
#include <QuartzCore/QuartzCore.h>

class MetalScene : public Scene
{
public:
    MetalScene(id<MTLDevice> device, id<MTLCommandQueue> queue);
    virtual ~MetalScene();

    virtual void createRayTracingHierarchy() override;
    
    id<MTLAccelerationStructure> newAccelerationStructureWithDescriptor(MTLAccelerationStructureDescriptor* descriptor);

    id<MTLDevice> device;
    id<MTLCommandQueue> queue;

    id<MTLBuffer> indicesBuffer;
    id<MTLBuffer> positionBuffer;
    id<MTLBuffer> texCoordsBuffer;
    id<MTLBuffer> normalBuffer;
    id<MTLBuffer> modelRefsBuffer;
    id<MTLBuffer> directionalLightBuffer;
    id<MTLBuffer> pointLightBuffer;
    id<MTLBuffer> instanceBuffer;

    id<MTLAccelerationStructure> accelerationStructure;
};
