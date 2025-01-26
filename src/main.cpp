#include "scene/Renderer.h"
#include "util/ModelLoader.h"
#include "window/Window.h"
#include <imgui.h>
#include <iostream>

int main()
{
  Renderer scene;
  Window window(1920, 1080);
  Camera camera = Camera{
      .position = glm::vec3(20, 5, 5),
      .target = glm::vec3(0, 0, 0),
  };
  RenderParameter render = RenderParameter{
      .width = 1920,
      .height = 1080,
      .numSamples = 10000,
  };
  scene.startRender(camera, render);

  while (true)
  {
    window.beginFrame();
    ImGui::Text("Camera Parameters");
    ImGui::InputFloat3("Position", &camera.position.x);
    ImGui::InputFloat3("Target", &camera.target.x);
    ImGui::InputFloat("Focal Length", &camera.f);
    ImGui::InputFloat("Aperture", &camera.A);
    ImGui::InputFloat("S_O", &camera.S_O);
    ImGui::Text("Render Parameters");
    ImGui::InputInt2("Dimensions", &render.width);
    ImGui::InputInt("Samples", &render.numSamples);
    if (ImGui::Button("Render"))
    {
      scene.startRender(camera, render);
    }
    ImGui::Text("Render Stats");
    ImGui::Text("Last Sample Time:    %.3f", scene.getLastSampleTime());
    ImGui::Text("Average Sample Time: %.3f", scene.getAverageSampleTime());
    ImGui::PlotLines("Sample Times", scene.getSampleTimes().data(), scene.getSampleTimes().size(), 0, 0, FLT_MAX, FLT_MAX, ImVec2(0, 40));
    window.update(scene.getImage());
  }
  return 0;
}