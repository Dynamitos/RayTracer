#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

std::vector<PModel> ModelLoader::loadModel(std::string_view filename)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename, 0);
    for (int m = 0; m < scene->mNumMeshes; ++m)
    {
        PModel result = std::make_unique<Model>();
        const aiMesh* mesh = scene->mMeshes[m];
        for (int v = 0; v < mesh->mNumVertices; ++v)
        {
            auto aiVert = mesh->mVertices[v];
            result->positions.add(glm::vec3(aiVert.x, aiVert.y, aiVert.z));
        }
        for (int i = 0; i < mesh->mNumFaces; ++i)
        {
            auto face = mesh->mFaces[i];
            result->indices.add(face.mIndices[0]);
            result->indices.add(face.mIndices[1]);
            result->indices.add(face.mIndices[2]);
        }
    }
}
