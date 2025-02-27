#include "MetalScene.h"

MetalScene::MetalScene(MTL::Device* device, MTL::CommandQueue* queue) : device(device), queue(queue) {}

MetalScene::~MetalScene() {}

void MetalScene::createRayTracingHierarchy()
{
  indicesBuffer = device->newBuffer(indicesPool.size() * sizeof(decltype(indicesPool)::value_type), MTL::ResourceStorageModeShared);
  positionBuffer = device->newBuffer(positionPool.size() * sizeof(decltype(positionPool)::value_type), MTL::ResourceStorageModeShared);
  texCoordsBuffer = device->newBuffer(texCoordsPool.size() * sizeof(decltype(texCoordsPool)::value_type), MTL::ResourceStorageModeShared);
  normalBuffer = device->newBuffer(normalsPool.size() * sizeof(decltype(normalsPool)::value_type), MTL::ResourceStorageModeShared);
  modelRefsBuffer = device->newBuffer(refs.size() * sizeof(decltype(refs)::value_type), MTL::ResourceStorageModeShared);
  if (directionalLights.size() > 0)
  {
    directionalLightBuffer =
        device->newBuffer(directionalLights.size() * sizeof(decltype(directionalLights)::value_type), MTL::ResourceStorageModeShared);
    std::memcpy(directionalLightBuffer->contents(), directionalLights.data(),
                directionalLights.size() * sizeof(decltype(directionalLights)::value_type));
  }
  if (pointLights.size() > 0)
  {
    pointLightBuffer = device->newBuffer(pointLights.size() * sizeof(decltype(pointLights)::value_type), MTL::ResourceStorageModeShared);
    std::memcpy(pointLightBuffer->contents(), pointLights.data(), pointLights.size() * sizeof(decltype(pointLights)::value_type));
  }

  std::memcpy(indicesBuffer->contents(), indicesPool.data(), indicesPool.size() * sizeof(decltype(indicesPool)::value_type));
  std::memcpy(positionBuffer->contents(), positionPool.data(), positionPool.size() * sizeof(decltype(positionPool)::value_type));
  std::memcpy(texCoordsBuffer->contents(), texCoordsPool.data(), texCoordsPool.size() * sizeof(decltype(texCoordsPool)::value_type));
  std::memcpy(normalBuffer->contents(), normalsPool.data(), normalsPool.size() * sizeof(decltype(normalsPool)::value_type));
  std::memcpy(modelRefsBuffer->contents(), refs.data(), refs.size() * sizeof(decltype(refs)::value_type));
    
    CFTypeRef* descriptors = new CFTypeRef[refs.size()];
  MTL::AccelerationStructure** primitiveAccelerationStructures = new MTL::AccelerationStructure*[refs.size()];
  for (uint i = 0; i < refs.size(); ++i)
  {
    MTL::AccelerationStructureTriangleGeometryDescriptor* descriptor = MTL::AccelerationStructureTriangleGeometryDescriptor::descriptor();
    descriptor->setTriangleCount(refs[i].numIndices / 3);
    descriptor->setIndexBuffer(indicesBuffer);
    descriptor->setIndexBufferOffset(refs[i].indicesOffset * sizeof(glm::uvec3));
    descriptor->setIndexType(MTL::IndexTypeUInt32);
    descriptor->setVertexBuffer(positionBuffer);
    descriptor->setVertexBufferOffset(refs[i].positionOffset * sizeof(glm::vec3));

    MTL::PrimitiveAccelerationStructureDescriptor* primitiveDescriptor = MTL::PrimitiveAccelerationStructureDescriptor::descriptor();
    primitiveDescriptor->setGeometryDescriptors(NS::Array::array(descriptor));

    primitiveAccelerationStructures[i] = device->newAccelerationStructure(primitiveDescriptor);
    primitiveAccelerationStructures[i]->setLabel(NS::String::string("Primitive Structure", NS::ASCIIStringEncoding));
    std::cout << primitiveAccelerationStructures[i]->debugDescription()->cString(NS::ASCIIStringEncoding) << std::endl;
      descriptors[i] = (CFTypeRef)primitiveAccelerationStructures[i];
  }
  instanceBuffer =
      device->newBuffer(sizeof(MTL::AccelerationStructureInstanceDescriptor) * refs.size(), MTL::ResourceOptionCPUCacheModeDefault);

  MTL::AccelerationStructureInstanceDescriptor* instanceDescriptors =
      (MTL::AccelerationStructureInstanceDescriptor*)instanceBuffer->contents();
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

    instanceDescriptors[i].options = MTL::AccelerationStructureInstanceOptionOpaque;
    instanceDescriptors[i].mask = 0xff;
  }
    NS::Array* pGeoDescriptors = ( NS::Array* )( CFArrayCreate( kCFAllocatorDefault, descriptors, refs.size(), &kCFTypeArrayCallBacks ) );
  MTL::InstanceAccelerationStructureDescriptor* accelDesc = MTL::InstanceAccelerationStructureDescriptor::descriptor();
  accelDesc->setInstancedAccelerationStructures(pGeoDescriptors);
  accelDesc->setInstanceDescriptorBuffer(instanceBuffer);
  accelDesc->setInstanceCount(refs.size());

  MTL::AccelerationStructureSizes accelSizes = device->accelerationStructureSizes(accelDesc);
  MTL::AccelerationStructure* tempStructure = device->newAccelerationStructure(accelSizes.accelerationStructureSize);
  tempStructure->setLabel(NS::String::string("Temporary AS", NS::ASCIIStringEncoding));
  MTL::Buffer* scratchBuffer = device->newBuffer(accelSizes.buildScratchBufferSize, MTL::StorageModeManaged);
  MTL::CommandBuffer* cmdBuffer = queue->commandBuffer();
  MTL::AccelerationStructureCommandEncoder* encoder = cmdBuffer->accelerationStructureCommandEncoder();
  MTL::Buffer* compactedBuffer = device->newBuffer(sizeof(uint), MTL::ResourceOptionCPUCacheModeDefault);
  encoder->buildAccelerationStructure(tempStructure, accelDesc, scratchBuffer, 0);
  encoder->writeCompactedAccelerationStructureSize(tempStructure, compactedBuffer, 0);
  encoder->endEncoding();
  cmdBuffer->commit();
  cmdBuffer->waitUntilCompleted();
  uint compactedSize = *(uint*)compactedBuffer->contents();
  accelerationStructure = device->newAccelerationStructure(compactedSize);
  accelerationStructure->setLabel(NS::String::string("Instance AS", NS::ASCIIStringEncoding));
  cmdBuffer = queue->commandBuffer();
  encoder = cmdBuffer->accelerationStructureCommandEncoder();
  encoder->copyAndCompactAccelerationStructure(tempStructure, accelerationStructure);
  encoder->endEncoding();
  cmdBuffer->commit();
  cmdBuffer->waitUntilCompleted();
}
