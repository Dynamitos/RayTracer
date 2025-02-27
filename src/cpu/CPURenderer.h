#pragma once
#include "cpu/CPUScene.h"
#include "scene/Renderer.h"
#include "util/Camera.h"
#include "ThreadPool.h"
#include <GL/glew.h>
#include <glfw/glfw3.h>

class CPURenderer : public Renderer
{
public:
    CPURenderer();
    virtual ~CPURenderer();
    
    virtual void addPointLight(PointLight point) override { scene->addPointLight(point); }
    virtual void addDirectionalLight(DirectionalLight dir) override { scene->addDirectionalLight(dir); }
    virtual void addModel(PModel model, glm::mat4 transform) override { scene->addModel(std::move(model), transform); }
    virtual void addModels(std::vector<PModel> models, glm::mat4 transform) override { scene->addModels(std::move(models), transform); }
    virtual void generate() override { scene->generate(); }

    virtual void beginFrame() override;
    virtual void update() override;
protected:
    virtual void render(Camera camera, RenderParameter params) override;
    CPUScene* scene;
    ThreadPool threadPool;
    // radiance accumulator
    std::vector<glm::vec3> accumulator;
    // the thing being displayed
    std::vector<glm::vec3> image;
    int width;
    int height;
    GLuint vao;
    GLuint texture;
    GLuint program;
    GLuint vertShader;
    GLuint fragShader;
    GLFWwindow* window;
};