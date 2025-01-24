#include "Window.h"
#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLSL(...) "#version 400\n" #__VA_ARGS__

Window::Window(int width, int height) : width(width), height(height)
{
    glewExperimental = true;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
    window = glfwCreateWindow(width, height, "RayTracer", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    ImGui_ImplGlfw_InitForOpenGL(window,
                                 true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    glewInit();
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    program = glCreateProgram();
    vertShader = glCreateShader(GL_VERTEX_SHADER);
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    int logLen = 0;
    const char* vertCode =
        GLSL(out vec2 texcoords; // texcoords are in the normalized [0,1] range for the viewport-filling quad part of the triangle
             void main() {
                 vec2 vertices[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));
                 gl_Position = vec4(vertices[gl_VertexID], 0, 1);
                 texcoords = 0.5 * gl_Position.xy + vec2(0.5);
             });
    const char* fragCode = GLSL(in vec2 texcoords; out vec4 color;

                                uniform sampler2D tex;

                                void main() { color = texture(tex, texcoords); });
    glShaderSource(vertShader, 1, &vertCode, nullptr);
    glCompileShader(vertShader);
    int success;
    char infoLog[1024];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertShader, 1024, NULL, infoLog);
        std::cout << "ERROR::SHADER_COMPILATION_ERROR\n"
                  << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
    }

    glShaderSource(fragShader, 1, &fragCode, nullptr);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragShader, 1024, NULL, infoLog);
        std::cout << "ERROR::SHADER_COMPILATION_ERROR\n"
                  << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
    }

    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &logLen);
    if (logLen > 0)
    {
        std::vector<char> log(logLen);
        glGetProgramInfoLog(program, logLen, &logLen, log.data());
        std::cout << log.data() << std::endl;
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glClearColor(0, 0, 0, 0);
}

Window::~Window() {}

void Window::beginFrame()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Window::update(const std::vector<glm::vec3>& textureData)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, textureData.data());
    glUseProgram(program);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}
