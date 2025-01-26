#include "scene/Renderer.h"
#include "util/ModelLoader.h"
#include "window/Window.h"
#include <iostream>
#include <imgui.h>

int main()
{
    Renderer scene;
    Window window(1920, 1080);
    scene.startRender(
        Camera{
            .position = glm::vec3(3, 3, 3),
            .direction = glm::vec3(-1, -1, -1),
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
                  .position = glm::vec3(5, 5, 5),
                  .direction = glm::vec3(-1, -1, -1),
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