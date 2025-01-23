#include "Window.h"
#include <assert.h>
#include <iostream>

#define GLSL(...) "#version 450\n#extension GL_KHR_vulkan_glsl : enable\n" #__VA_ARGS__

Window::Window(int width, int height) : width(width), height(height)
{
    glewExperimental = true;
    assert(glfwInit());
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
    window = glfwCreateWindow(width, height, "RayTracer", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    float vertices[] = {
        // positions        
        1,  -1, 0,
        -1, -1, 0,
        1, 1, 0,
        1, -1, 0
    };
    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    assert(!glewInit());
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    program = glCreateProgram();
    vertShader = glCreateShader(GL_VERTEX_SHADER);
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    int logLen = 0;
    const char* vertCode = GLSL(layout(location = 0) in vec3 position
        layout(location = 0) out vec2 outUV;

                                void main() {
                                    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
                                    gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
                                });
    const char* fragCode = GLSL(layout(location = 0) in vec2 uv; layout(location = 0) out vec4 color;

                                uniform sampler2D tex;

                                void main() { color = texture(tex, uv); });
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
    textureLocation = glGetUniformLocation(program, "tex");
    glClearColor(0, 0, 0, 0);
}

Window::~Window() {}

void Window::update(const std::vector<uint32_t>& textureData)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data());
    glUseProgram(program);
    glUniform1i(textureLocation, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glfwSwapBuffers(window);
    glfwPollEvents();
}
