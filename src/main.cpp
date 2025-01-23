#include "window/Window.h"

int main() {
	Window w(1920, 1080);
	while (true) {
		w.update();
	}
	return 0;
}