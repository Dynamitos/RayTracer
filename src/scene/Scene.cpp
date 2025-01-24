#include "Scene.h"

Scene::Scene(glm::vec3 cameraPos, glm::vec3 cameraDirection) : cameraPos(cameraPos), cameraDirection(cameraDirection) {}

Scene::~Scene() {}

void Scene::render(int width, int height, int numSamples)
{
    image.clear();
    accumulator.clear();
    image.resize(width * height * 3);
    accumulator.resize(width * height * 3);
    for (int samp = 0; samp < numSamples; ++samp)
    {
        for (int w = 0; w < width; ++w)
        {
            for (int h = 0; h < height; ++h)
            {
            }
        }
    }
}
