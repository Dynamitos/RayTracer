target_sources(RayTracer
    PRIVATE
        MetalRenderer.h
        MetalRenderer.mm
        MetalScene.h
        MetalScene.mm
        Compute.metal
        PrivateImpl.mm)