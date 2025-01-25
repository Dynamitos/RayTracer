#include "scene/BVH.h"
#include "scene/Scene.h"
#include "util/ModelLoader.h"
#include "window/Window.h"
#include <iostream>
#include <imgui.h>

int main()
{
    Scene scene;
    Window window(1920, 1080);
    scene.startRender(
        Camera{
            .position = glm::vec3(0, 10, 10),
            .direction = glm::vec3(0, -1, -1),
        },
        RenderParameter{
            .width = 1920,
            .height = 1080,
            .numSamples = 10000,
        });
    while (true)
    {
        window.beginFrame();
        if (ImGui::Button("Render"))
        {
          scene.startRender(
              Camera{
                  .position = glm::vec3(0, 10, 10),
                  .direction = glm::vec3(0, -1, -1),
              },
              RenderParameter{
                  .width = 1920,
                  .height = 1080,
                  .numSamples = 10000,
              });
        }
        window.update(scene.getImage());
    }
    return 0;
}