#include "Scene.h"
#include "util/ModelLoader.h"
#include <chrono>
#include <iostream>

Scene::Scene()
{
  bvh.addModels(ModelLoader::loadModel("../res/models/cube.fbx"), glm::mat4(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                                                                 glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
  bvh.generate();
}

Scene::~Scene() {}

static bool firstTime = true;
void Scene::startRender(Camera cam, RenderParameter params)
{
  if (!firstTime)
  {
    pendingCancel = true;
    worker.join();
    firstTime = false;
  }
  pendingCancel = false;
  image.clear();
  accumulator.clear();
  image.resize(params.width * params.height);
  accumulator.resize(params.width * params.height);
  worker = std::thread(&Scene::render, this, cam, params);
}

void Scene::render(Camera cam, RenderParameter params)
{
  for (int samp = 0; samp < params.numSamples; ++samp)
  {
    if (pendingCancel)
      return;
    Batch batch;
    for (int w = 0; w < params.width; ++w)
    {
      batch.jobs.push_back(
          [&, w]()
          {
            for (int h = 0; h < params.height; ++h)
            {
              Ray r = Ray{.origin = glm::vec3(0.0f),
                          .direction = glm::normalize(
                              glm::vec3((float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX))};
              bvh.traceRay(r);

              accumulator[w + h * params.width] +=
                  glm::vec3(w / float(params.width * params.numSamples), h / float(params.height * params.numSamples), 0);
            }
          });
    }
    auto start = std::chrono::high_resolution_clock::now();
    threadPool.runBatch(std::move(batch));
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    std::memcpy(image.data(), accumulator.data(), accumulator.size() * sizeof(glm::vec3));
  }
}
