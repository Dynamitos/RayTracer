#include "Scene.h"

void Scene::addModel(PModel model, glm::mat4 transform)
{
  model->transform(transform);
  models.push_back(std::move(model));
}

void Scene::addModels(std::vector<PModel> _models, glm::mat4 transform)
{
  for (auto& _model : _models)
  {
    _model->transform(transform);
    models.push_back(std::move(_model));
  }
}

void Scene::generate()
{
  // todo: clear everything
  for (uint32_t i = 0; i < models.size(); ++i)
  {
    auto& model = models[i];
    ModelReference ref = {
        .positionOffset = (uint32_t)positionPool.size(),
        .numPositions = (uint32_t)model->positions.size(),
        .indicesOffset = (uint32_t)indicesPool.size(),
        .numIndices = (uint32_t)model->indices.size(),
    };
    for (uint32_t i = 0; i < model->positions.size(); ++i)
    {
      positionPool.push_back(model->positions[i]);
      texCoordsPool.push_back(model->texCoords[i]);
      normalsPool.push_back(model->normals[i]);
    }
    for (uint32_t i = 0; i < model->indices.size(); ++i)
    {
      indicesPool.push_back(model->indices[i]);
      edgesPool.push_back(model->edges[i * 2 + 0]);
      edgesPool.push_back(model->edges[i * 2 + 1]);
      faceNormalsPool.push_back(glm::normalize(model->faceNormals[i]));
    }
    refs.push_back(ref);
  }
  createRayTracingHierarchy();
}
