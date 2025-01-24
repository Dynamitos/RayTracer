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
    scene.render(
        Camera{
            .position = glm::vec3(10, 0, 0),
            .direction = glm::vec3(-1, 0, 0),
        },
        RenderParameter{
            .width = 1920,
            .height = 1080,
            .numSamples = 10000,
        });
    while (true)
    {
        window.beginFrame();
        if (ImGui::Button("Test"))
        {
            std::cout << "Test" << std::endl;
        }
        window.update(scene.getImage());
    }
    return 0;
}