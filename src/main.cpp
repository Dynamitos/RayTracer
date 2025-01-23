#include "window/Window.h"
#include "util/ModelLoader.h"
#include "scene/BVH.h"

int main() {
    BVH bvh;
    bvh.addModels(ModelLoader::loadModel("../../cube.fbx"));
    bvh.generate();
	Window w(1920, 1080);
	while (true) {
		w.update();
	}
	return 0;
}