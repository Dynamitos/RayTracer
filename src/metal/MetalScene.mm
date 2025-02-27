#include "MetalScene.h"

MetalScene::MetalScene(id<MTLDevice> device, id<MTLCommandQueue> queue) : device(device), queue(queue) {}

MetalScene::~MetalScene() {}

void MetalScene::createRayTracingHierarchy()
{
  indicesBuffer = [device newBufferWithLength:indicesPool.size() * sizeof(decltype(indicesPool)::value_type) options:MTLResourceStorageModeShared];
  positionBuffer = [device newBufferWithLength:positionPool.size() * sizeof(decltype(positionPool)::value_type) options:MTLResourceStorageModeShared];
  texCoordsBuffer = [device newBufferWithLength:texCoordsPool.size() * sizeof(decltype(texCoordsPool)::value_type) options:MTLResourceStorageModeShared];
  normalBuffer = [device newBufferWithLength:normalsPool.size() * sizeof(decltype(normalsPool)::value_type) options:MTLResourceStorageModeShared];
  modelRefsBuffer = [device newBufferWithLength:refs.size() * sizeof(decltype(refs)::value_type) options:MTLResourceStorageModeShared];
  if (directionalLights.size() > 0)
  {
    directionalLightBuffer =
        [device newBufferWithLength:directionalLights.size() * sizeof(decltype(directionalLights)::value_type) options:MTLResourceStorageModeShared];
    std::memcpy(directionalLightBuffer.contents, directionalLights.data(),
                directionalLights.size() * sizeof(decltype(directionalLights)::value_type));
  }
  if (pointLights.size() > 0)
  {
    pointLightBuffer = [device newBufferWithLength:pointLights.size() * sizeof(decltype(pointLights)::value_type) options:MTLResourceStorageModeShared];
    std::memcpy(pointLightBuffer.contents, pointLights.data(), pointLights.size() * sizeof(decltype(pointLights)::value_type));
  }

  std::memcpy(indicesBuffer.contents, indicesPool.data(), indicesPool.size() * sizeof(decltype(indicesPool)::value_type));
  std::memcpy(positionBuffer.contents, positionPool.data(), positionPool.size() * sizeof(decltype(positionPool)::value_type));
  std::memcpy(texCoordsBuffer.contents, texCoordsPool.data(), texCoordsPool.size() * sizeof(decltype(texCoordsPool)::value_type));
  std::memcpy(normalBuffer.contents, normalsPool.data(), normalsPool.size() * sizeof(decltype(normalsPool)::value_type));
  std::memcpy(modelRefsBuffer.contents, refs.data(), refs.size() * sizeof(decltype(refs)::value_type));
    
  NSMutableArray* primitiveStructures = [[NSMutableArray alloc] init];
  for (uint i = 0; i < refs.size(); ++i)
  {
    MTLAccelerationStructureTriangleGeometryDescriptor* descriptor = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
    descriptor.triangleCount = refs[i].numIndices;
    descriptor.indexBuffer = indicesBuffer;
    descriptor.indexBufferOffset = refs[i].indicesOffset * sizeof(glm::uvec3);
    descriptor.indexType = MTLIndexTypeUInt32;
    descriptor.vertexBuffer = positionBuffer;
    descriptor.vertexBufferOffset = refs[i].positionOffset * sizeof(glm::vec3);

    MTLPrimitiveAccelerationStructureDescriptor* primitiveDescriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
    primitiveDescriptor.geometryDescriptors = @[ descriptor ];

      id<MTLAccelerationStructure> accelerationStructure = newAccelerationStructureWithDescriptor(primitiveDescriptor);
      [accelerationStructure setLabel:@"Primitive Structure"];
      [primitiveStructures addObject:accelerationStructure];
  }
  instanceBuffer =
    [device newBufferWithLength:sizeof(MTLAccelerationStructureInstanceDescriptor) * refs.size() options:MTLResourceStorageModeShared];

  MTLAccelerationStructureInstanceDescriptor* instanceDescriptors =
      (MTLAccelerationStructureInstanceDescriptor*)instanceBuffer.contents;
  for (uint i = 0; i < refs.size(); ++i)
  {
    instanceDescriptors[i].transformationMatrix[0][0] = 1.0f;
    instanceDescriptors[i].transformationMatrix[1][0] = 0.0f;
    instanceDescriptors[i].transformationMatrix[2][0] = 0.0f;
    instanceDescriptors[i].transformationMatrix[3][0] = 0.0f;

    instanceDescriptors[i].transformationMatrix[0][1] = 0.0f;
    instanceDescriptors[i].transformationMatrix[1][1] = 1.0f;
    instanceDescriptors[i].transformationMatrix[2][1] = 0.0f;
    instanceDescriptors[i].transformationMatrix[3][1] = 0.0f;

    instanceDescriptors[i].transformationMatrix[0][2] = 0.0f;
    instanceDescriptors[i].transformationMatrix[1][2] = 0.0f;
    instanceDescriptors[i].transformationMatrix[2][2] = 1.0f;
    instanceDescriptors[i].transformationMatrix[3][2] = 0.0f;

    instanceDescriptors[i].accelerationStructureIndex = i;

    instanceDescriptors[i].options = MTLAccelerationStructureInstanceOptionOpaque;
    instanceDescriptors[i].mask = 0xff;
  }
  MTLInstanceAccelerationStructureDescriptor* accelDesc = [MTLInstanceAccelerationStructureDescriptor descriptor];
  [accelDesc setInstancedAccelerationStructures:primitiveStructures];
  [accelDesc setInstanceDescriptorBuffer:instanceBuffer];
  [accelDesc setInstanceCount:refs.size()];
    accelerationStructure = newAccelerationStructureWithDescriptor(accelDesc);
}

id<MTLAccelerationStructure> MetalScene::newAccelerationStructureWithDescriptor(MTLAccelerationStructureDescriptor* descriptor)
{
    // Query for the sizes needed to store and build the acceleration structure.
    MTLAccelerationStructureSizes accelSizes = [device accelerationStructureSizesWithDescriptor:descriptor];

    // Allocate an acceleration structure large enough for this descriptor. This method
    // doesn't actually build the acceleration structure, but rather allocates memory.
    id <MTLAccelerationStructure> accelerationStructure = [device newAccelerationStructureWithSize:accelSizes.accelerationStructureSize];

    // Allocate scratch space Metal uses to build the acceleration structure.
    // Use MTLResourceStorageModePrivate for the best performance because the sample
    // doesn't need access to buffer's contents.
    id <MTLBuffer> scratchBuffer = [device newBufferWithLength:accelSizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];

    // Create a commandbuffer that performs the acceleration structure build.
    id <MTLCommandBuffer> commandBuffer = [queue commandBuffer];

    // Create an acceleration structure command encoder.
    id <MTLAccelerationStructureCommandEncoder> commandEncoder = [commandBuffer accelerationStructureCommandEncoder];

    // Allocate a buffer for Metal to write the compacted accelerated structure's size into.
    id <MTLBuffer> compactedSizeBuffer = [device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];

    // Schedule the actual acceleration structure build.
    [commandEncoder buildAccelerationStructure:accelerationStructure
                                    descriptor:descriptor
                                 scratchBuffer:scratchBuffer
                           scratchBufferOffset:0];

    // Compute and write the compacted acceleration structure size into the buffer. You
    // must already have a built acceleration structure because Metal determines the compacted
    // size based on the final size of the acceleration structure. Compacting an acceleration
    // structure can potentially reclaim significant amounts of memory because Metal must
    // create the initial structure using a conservative approach.

    [commandEncoder writeCompactedAccelerationStructureSize:accelerationStructure
                                                   toBuffer:compactedSizeBuffer
                                                     offset:0];

    // End encoding, and commit the command buffer so the GPU can start building the
    // acceleration structure.
    [commandEncoder endEncoding];

    [commandBuffer commit];

    // The sample waits for Metal to finish executing the command buffer so that it can
    // read back the compacted size.

    // Note: Don't wait for Metal to finish executing the command buffer if you aren't compacting
    // the acceleration structure, as doing so requires CPU/GPU synchronization. You don't have
    // to compact acceleration structures, but do so when creating large static acceleration
    // structures, such as static scene geometry. Avoid compacting acceleration structures that
    // you rebuild every frame, as the synchronization cost may be significant.

    [commandBuffer waitUntilCompleted];

    uint32_t compactedSize = *(uint32_t *)compactedSizeBuffer.contents;

    // Allocate a smaller acceleration structure based on the returned size.
    id <MTLAccelerationStructure> compactedAccelerationStructure = [device newAccelerationStructureWithSize:compactedSize];

    // Create another command buffer and encoder.
    commandBuffer = [queue commandBuffer];

    commandEncoder = [commandBuffer accelerationStructureCommandEncoder];

    // Encode the command to copy and compact the acceleration structure into the
    // smaller acceleration structure.
    [commandEncoder copyAndCompactAccelerationStructure:accelerationStructure
                                toAccelerationStructure:compactedAccelerationStructure];

    // End encoding and commit the command buffer. You don't need to wait for Metal to finish
    // executing this command buffer as long as you synchronize any ray-intersection work
    // to run after this command buffer completes. The sample relies on Metal's default
    // dependency tracking on resources to automatically synchronize access to the new
    // compacted acceleration structure.
    [commandEncoder endEncoding];
    [commandBuffer commit];

    return compactedAccelerationStructure;
}
