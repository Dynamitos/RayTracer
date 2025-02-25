target_sources(RayTracer
    PRIVATE
        MetalRenderer.h
        MetalRenderer.cpp
        MetalScene.h
        MetalScene.cpp
        Compute.metal
        PrivateImpl.mm)