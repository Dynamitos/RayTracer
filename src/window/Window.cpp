#include "Window.h"

Window::Window(int width, int height)
{
	glfwInit();
	window = glfwCreateWindow(width, height, "RayTracer", nullptr, nullptr);
}

Window::~Window()
{
}

void Window::update()
{
	glfwPollEvents();
	glfwSwapBuffers(window);
}
