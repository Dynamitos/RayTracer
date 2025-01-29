#include "Model.h"

void Model::transform(glm::mat4 matrix)
{
  for (auto& pos : positions)
  {
    pos = glm::vec3(matrix * glm::vec4(pos, 1));
  }

  for (auto& nor : normals)
  {
    nor = glm::mat3(matrix) * nor;
  }

  boundingBox.transform(matrix);

  for (int i = 0; i < indices.size(); i++)
  {
    auto e0 = positions[indices[i].y] - positions[indices[i].x];
    auto e1 = positions[indices[i].z] - positions[indices[i].x];
    edges.push_back(e0);
    edges.push_back(e1);

    faceNormals.push_back(glm::cross(e0, e1));
  }
}
