#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

std::vector<PModel> ModelLoader::loadModel(std::string_view filename)
{
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(std::string(filename), aiProcess_Triangulate | aiProcess_GenNormals);
  std::cout << importer.GetErrorString() << std::endl;
  std::vector<PModel> result;
  for (int m = 0; m < scene->mNumMeshes; ++m)
  {
    PModel model = std::make_unique<Model>();
    const aiMesh* mesh = scene->mMeshes[m];
    AABB aabb;
    for (int v = 0; v < mesh->mNumVertices; ++v)
    {
      auto aiVert = mesh->mVertices[v];
      model->positions.push_back(glm::vec3(aiVert.x, aiVert.y, aiVert.z));
      if (mesh->HasTextureCoords(0))
      {
        model->texCoords.push_back(glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y));
      }
      else
      {
        model->texCoords.push_back(glm::vec2(0, 0));
      }
      model->normals.push_back(glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z));
      aabb.adjust(model->positions.back());
    }
    for (int i = 0; i < mesh->mNumFaces; ++i)
    {
      auto face = mesh->mFaces[i];
      model->indices.push_back(glm::uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]));
    }
    model->boundingBox = aabb;
    result.push_back(std::move(model));
  }
  return result;
}
