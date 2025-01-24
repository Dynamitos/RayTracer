#include "scene/BVH.h"
#include "util/ModelLoader.h"
#include "window/Window.h"

int main()
{
    Scene scene;
    Window window;
    while (true)
    {
        // IMGUI....
        if (Imgui.Button())
        {
            scene.render();
        }
        window.update(scene.getImage());
    }
    return 0;
}