#include "Scene.h"
#include "util/ModelLoader.h"
#include <chrono>
#include <iostream>
#include <random>

Scene::Scene()
{
  bvh.addModels(ModelLoader::loadModel("../res/models/cube.fbx"),
                glm::mat4(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                          glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
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

void Scene::render(Camera camera, RenderParameter params)
{
  std::random_device rd;
  for (int samp = 0; samp < params.numSamples; ++samp)
  {
    if (pendingCancel)
      return;
    Batch batch;
    for (int w = 0; w < params.width; ++w)
    {
      batch.jobs.push_back(
          [&](int w) -> Task
          {
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> rnd01(0.0, 1.0);
            std::uniform_real_distribution<float> rnd02(0.0, 2.0);
            for (int h = 0; h < params.height; ++h)
            {
              Ray cam = Ray(camera.position, camera.direction);
              glm::vec3 cx =
                            glm::normalize(glm::cross(cam.direction, abs(cam.direction.y) < 0.9 ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1))),
                        cy = glm::cross(cx, cam.direction);
              const glm::vec2 sdim = camera.sensorSize; // sensor size (36 x 24 mm)

              float S_I = (camera.S_O * camera.f) / (camera.S_O - camera.f);

              //-- sample sensor
              glm::uvec2 pix = glm::uvec2(w, h);
              glm::vec2 rnd2 = glm::vec2(rnd02(gen), rnd02(gen)); // vvv tent filter sample
              glm::vec2 tent =
                  glm::vec2(rnd2.x < 1 ? sqrt(rnd2.x) - 1 : 1 - sqrt(2 - rnd2.x), rnd2.y < 1 ? sqrt(rnd2.y) - 1 : 1 - sqrt(2 - rnd2.y));
              glm::vec2 s =
                  ((glm::vec2(pix) + 0.5f * (0.5f + glm::vec2((samp / 2) % 2, samp % 2) + tent)) / glm::vec2(params.width, params.height) -
                   0.5f) *
                  sdim;
              glm::vec3 spos = cam.origin + cx * s.x + cy * s.y, lc = cam.origin + cam.direction * 0.035f; // sample on 3d sensor plane
              glm::vec3 accrad = glm::vec3(0), accmat = glm::vec3(1); // initialize accumulated radiance and bxdf
              Ray r = Ray(lc, normalize(lc - spos));                  // construct ray

              //-- setup lens
              glm::vec3 lensP = lc;
              glm::vec3 lensN = -cam.direction;
              glm::vec3 lensX = glm::cross(lensN, glm::vec3(0, 1, 0)); // the exact vector doesnt matter
              glm::vec3 lensY = glm::cross(lensN, lensX);

              glm::vec3 lensSample = lensP + rnd01(gen) * camera.A * lensX + rnd01(gen) * camera.A * lensY;

              glm::vec3 focalPoint = cam.origin + (camera.S_O + S_I) * cam.direction;
              float t = glm::dot(focalPoint - r.origin, lensN) / glm::dot(r.direction, lensN);
              glm::vec3 focus = r.origin + t * r.direction;
              r = Ray(lensSample, normalize(focus - lensSample)); // TODO: Fix lens

              auto intersection = bvh.traceRay(r);

              if (intersection.has_value())
              {
                accumulator[w + h * params.width] = intersection->albedo;
              }
            }
            co_return;
          }(w));
    }
    auto start = std::chrono::high_resolution_clock::now();
    threadPool.runBatch(std::move(batch));
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    std::memcpy(image.data(), accumulator.data(), accumulator.size() * sizeof(glm::vec3));
  }
}
