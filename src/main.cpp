#include "scene/BVH.h"
#include "util/ModelLoader.h"
#include "window/Window.h"

int main()
{
    BVH bvh;
    bvh.addModels(ModelLoader::loadModel("../cube.fbx"), glm::mat4());
    bvh.generate();
    Window w(1920, 1080);
    std::vector<unsigned char> texture(1920 * 1080 * 3);
    for (int j = 10; j < 100; ++j)
    {
        for (int i = 0; i < 1920; ++i)
        {
            texture[(j * 1920 + i) * 3] = 0xff;
            texture[(j * 1920 + i) * 3 + 1] = 0xff;
            texture[(j * 1920 + i) * 3 + 2] = 0x0;
        }
    }
    while (true)
    {
        w.update(texture);
    }
    return 0;
}