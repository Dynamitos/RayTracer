#include "Scene.h"

Scene::Scene(glm::vec3 cameraPos, glm::vec3 cameraDirection) : cameraPos(cameraPos), cameraDirection(cameraDirection) {}

Scene::~Scene() {}

void Scene::render(int width, int height, int numSamples)
{
    worker = std::thread(
        [&]()
        {
            image.clear();
            accumulator.clear();
            image.resize(width * height * 3);
            for (int samp = 0; samp < numSamples; ++samp)
            {
                std::function<void()> job = [&]()
                {
                    std::vector<unsigned char> localAccumulator(width * height * 3);
                    for (int w = 0; w < width; ++w)
                    {
                        for (int h = 0; h < height; ++h)
                        {
                            if (cancel)
                                return;
                            bvh.traceRay();
                        }
                    }
                    // lock image
                    // add result to image
                };
            }
        });
}
