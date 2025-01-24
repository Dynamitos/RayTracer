#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

std::vector<PModel> ModelLoader::loadModel(std::string_view filename)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(std::string(filename), aiProcess_Triangulate);
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
            aabb.adjust(model->positions.back());
        }
        for (int i = 0; i < mesh->mNumFaces; ++i)
        {
            auto face = mesh->mFaces[i];
            model->indices.push_back(face.mIndices[0]);
            model->indices.push_back(face.mIndices[1]);
            model->indices.push_back(face.mIndices[2]);

            auto e0 = model->positions[face.mIndices[1]] - model->positions[face.mIndices[0]];
            auto e1 = model->positions[face.mIndices[2]] - model->positions[face.mIndices[0]];
            model->es.push_back(e0);
            model->es.push_back(e1);

            model->faceNormals.push_back(glm::cross(e0, e1));
        }
        model->boundingBox = aabb;
        result.push_back(std::move(model));
    }
    return result;
}
