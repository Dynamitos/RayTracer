#pragma once
#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <vector>

class Window
{
  public:
    Window(int width, int height);
    ~Window();
    void update(const std::vector<unsigned char>& textureData);

  private:
    int width;
    int height;
    GLuint vao;
    GLuint texture;
    GLuint program;
    GLuint vertShader;
    GLuint fragShader;
    GLuint textureLocation;
    GLFWwindow* window;
};