#include "Renderer.h"


Renderer::Renderer()
{
}

Renderer::~Renderer() {}

void Renderer::startRender(Camera cam, RenderParameter params)
{
  //threadPool.cancel();
  if (running)
  {
    running = false;
    worker.join();
  }
  sampleTimes.clear();
  image.clear();
  image.resize(params.width * params.height);
  running = true;
  worker = std::thread(&Renderer::render, this, cam, params);
}