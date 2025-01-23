#pragma once
#include <glfw/glfw3.h>

class Window
{
public:
	Window(int width, int height);
	~Window();
	void update();
private:
	GLFWwindow* window;
};