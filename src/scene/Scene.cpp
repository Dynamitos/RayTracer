#include "Scene.h"

Scene::Scene() {}

Scene::~Scene() {}

void Scene::render(Camera cam, RenderParameter params)
{
    pendingCancel = true;
    worker.join();
    pendingCancel = false;
    worker = std::thread(
        [&]()
        {
            image.clear();
            accumulator.clear();
            image.resize(params.width * params.height * 3);
            accumulator.resize(params.width * params.height * 3);
            for (int samp = 0; samp < params.numSamples; ++samp)
            {
                if (pendingCancel)
                    return;
                Batch batch;
                for (int w = 0; w < params.width; ++w)
                {
                    for (int h = 0; h < params.height; ++h)
                    {
                        batch.jobs.push_back(
                            [&]()
                            {
                                Ray r = Ray();
                                bvh.traceRay(r);
                            });
                    }
                }
                threadPool.runBatch(std::move(batch));
                std::memcpy(image.data(), accumulator.data(), accumulator.size());
            }
        });
}
