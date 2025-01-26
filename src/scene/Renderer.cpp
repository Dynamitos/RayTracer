#include "Renderer.h"
#include "util/ModelLoader.h"
#include <chrono>
#include <iostream>
#include <random>

Renderer::Renderer()
{
  bvh.addDirectionalLight(DirectionalLight{
      .direction = glm::normalize(glm::vec3(-0.4f, -0.2f, -0.6f)),
      .color = glm::vec3(1, 1, 1),
  });
  bvh.addModels(ModelLoader::loadModel("../res/models/box.glb"),
                glm::mat4(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                          glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
  bvh.generate();
}

Renderer::~Renderer() {}

static bool firstTime = true;
void Renderer::startRender(Camera cam, RenderParameter params)
{
  threadPool.cancel();
  pendingCancel = true;
  if (worker.joinable())
    worker.join();
  pendingCancel = false;
  image.clear();
  accumulator.clear();
  image.resize(params.width * params.height);
  accumulator.resize(params.width * params.height);
  worker = std::thread(&Renderer::render, this, cam, params);
}

glm::vec3 rand01(glm::uvec3 x)
{ // pseudo-random number generator
  for (int i = 3; i-- > 0;)
    x = ((x >> 8U) ^ glm::uvec3(x.y, x.z, x.x)) * 1103515245U;
  return glm::vec3(x) * (1.0f / float(0xffffffffU));
}

thread_local glm::vec3 rnd01;

void Renderer::render(Camera camera, RenderParameter params)
{
  for (int samp = 0; samp < params.numSamples; ++samp)
  {
    if (pendingCancel)
      return;
    auto start = std::chrono::high_resolution_clock::now();
    Batch batch;
    for (int w = 0; w < params.width; ++w)
    {
      batch.jobs.push_back(
          [&](int w, int samp) -> Task
          {
            // #pragma omp parallel for
            for (int h = 0; h < params.height; ++h)
            {
              Ray cam = Ray(camera.position, glm::normalize(camera.direction));
              glm::vec3 cx =
                            glm::normalize(glm::cross(cam.direction, abs(cam.direction.y) < 0.9 ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1))),
                        cy = glm::cross(cx, cam.direction);
              const glm::vec2 sdim = camera.sensorSize; // sensor size (36 x 24 mm)

              float S_I = (camera.S_O * camera.f) / (camera.S_O - camera.f);

              //-- sample sensor
              glm::uvec2 pix = glm::uvec2(w, h);
              rnd01 = rand01(glm::uvec3(pix, samp));
              glm::vec2 rnd2 = 2.0f * glm::vec2(rnd01); // vvv tent filter sample
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

              glm::vec3 lensSample = lensP + rnd01.x * camera.A * lensX + rnd01.y * camera.A * lensY;

              glm::vec3 focalPoint = cam.origin + (camera.S_O + S_I) * cam.direction;
              float t = glm::dot(focalPoint - r.origin, lensN) / glm::dot(r.direction, lensN);
              glm::vec3 focus = r.origin + t * r.direction;
              r = Ray(lensSample, normalize(focus - lensSample)); // TODO: Fix lens

              Payload payload;
              bvh.traceRay(r, payload, 1e-4, 1e20);

              accumulator[w + h * params.width] += payload.accumulatedRadiance;
            }
            co_return;
          }(w, samp));
    }
    threadPool.runBatch(std::move(batch));
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    for (uint32_t i = 0; i < accumulator.size(); ++i)
    {
      image[i] = glm::pow(glm::max((accumulator[i] / float(samp)), 0.0f), glm::vec3(0.45f));
    }
  }
}
