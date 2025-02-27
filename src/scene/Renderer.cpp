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
  running = true;
  worker = std::thread(&Renderer::render, this, cam, params);
}