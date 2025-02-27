#include "metal/MetalRenderer.h"
#include "scene/Renderer.h"
#include "util/ModelLoader.h"
#include <imgui.h>

int main()
{
  std::unique_ptr<Renderer> renderer = std::make_unique<MetalRenderer>();

  renderer->addDirectionalLight(DirectionalLight{
      .direction = glm::normalize(glm::vec3(-0.4f, -0.3f, -0.2f)),
      .color = glm::vec3(1, 1, 1),
  });
  renderer->addPointLight(PointLight{});
  renderer->addModels(ModelLoader::loadModel("../../res/models/cube.fbx"),
                      glm::mat4(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                                glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
  renderer->addModels(ModelLoader::loadModel("../../res/models/cube.fbx"),
                      glm::mat4(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
                                glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
  renderer->generate();
  Camera camera = Camera{
      .position = glm::vec3(5, 1, 2),
      .target = glm::vec3(0, 0, 0),
      .S_O = 6,
  };
  RenderParameter render = RenderParameter{
      .width = 1920,
      .height = 1080,
      .numSamples = 10000,
  };
  renderer->startRender(camera, render);

  while (true)
  {
    renderer->beginFrame();
    ImGui::Text("Camera Parameters");
    ImGui::InputFloat3("Position", &camera.position.x);
    ImGui::InputFloat3("Target", &camera.target.x);
    ImGui::InputFloat("Focal Length", &camera.f);
    ImGui::InputFloat("Aperture", &camera.A);
    ImGui::InputFloat("S_O", &camera.S_O);
    ImGui::Text("Render Parameters");
    ImGui::InputInt2("Dimensions", (int*)&render.width);
    ImGui::InputInt("Samples", (int*)&render.numSamples);
    if (ImGui::Button("Render"))
    {
      renderer->startRender(camera, render);
    }
    ImGui::Text("Render Stats");
    ImGui::Text("Last Sample Time:    %.3f ms", renderer->getLastSampleTime());
    ImGui::Text("Average Sample Time: %.3f ms", renderer->getAverageSampleTime());
    ImGui::PlotLines("Sample Times", renderer->getSampleTimes().data(), renderer->getSampleTimes().size(), 0, 0, FLT_MAX, FLT_MAX,
                     ImVec2(0, 40));
    renderer->update();
  }
  return 0;
}
