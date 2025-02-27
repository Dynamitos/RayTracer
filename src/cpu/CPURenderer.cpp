#include "CPURenderer.h"
#include "scene/Renderer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLSL(...) "#version 400\n" #__VA_ARGS__

CPURenderer::CPURenderer()
{
  glewExperimental = true;
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
  window = glfwCreateWindow(width, height, "RayTracer", nullptr, nullptr);
  glfwSwapInterval(1);
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

                              void main() { color = texture(tex, vec2(texcoords.x, -texcoords.y)); });
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

glm::vec3 rand01(glm::uvec3 x)
{ // pseudo-random number generator
  for (int i = 3; i-- > 0;)
    x = ((x >> 8U) ^ glm::uvec3(x.y, x.z, x.x)) * 1103515245U;
  return glm::vec3(x) * (1.0f / float(0xffffffffU));
}

void CPURenderer::beginFrame()
{
  glClear(GL_COLOR_BUFFER_BIT);
  glfwPollEvents();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void CPURenderer::update()
{
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, image.data());
  glUseProgram(program);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window);
}

void CPURenderer::render(Camera camera, RenderParameter params)
{
  image.clear();
  image.resize(params.width * params.height);
  accumulator.clear();
  accumulator.resize(params.width * params.height);
  for (int samp = 0; samp < params.numSamples; ++samp)
  {
    if (!running)
      return;
    auto start = std::chrono::high_resolution_clock::now();
    Batch batch;
    for (int w = 0; w < params.width; ++w)
    {
      batch.jobs.push_back(
          [&](int w, int samp) -> Task
          {
            // #pragma omp parallel for
            for (int h = 0; h < params.height; ++h)
            {
              Payload payload;
              Ray cam = Ray(camera.position, glm::normalize(camera.target - camera.position));
              glm::vec3 cx =
                            glm::normalize(glm::cross(cam.direction, abs(cam.direction.y) < 0.9 ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1))),
                        cy = glm::cross(cx, cam.direction);
              const glm::vec2 sdim = camera.sensorSize; // sensor size (36 x 24 mm)

              float S_I = (camera.S_O * camera.f) / (camera.S_O - camera.f);

              //-- sample sensor
              glm::uvec2 pix = glm::uvec2(w, h);

              payload.rnd01 = rand01(glm::uvec3(pix, samp));
              glm::vec2 rnd2 = 2.0f * glm::vec2(payload.rnd01); // vvv tent filter sample
              glm::vec2 tent =
                  glm::vec2(rnd2.x < 1 ? sqrt(rnd2.x) - 1 : 1 - sqrt(2 - rnd2.x), rnd2.y < 1 ? sqrt(rnd2.y) - 1 : 1 - sqrt(2 - rnd2.y));
              glm::vec2 s =
                  ((glm::vec2(pix) + 0.5f * (0.5f + glm::vec2((samp / 2) % 2, samp % 2) + tent)) / glm::vec2(params.width, params.height) -
                   0.5f) *
                  sdim;
              glm::vec3 spos = cam.origin + cx * s.x + cy * s.y, lc = cam.origin + cam.direction * 0.035f; // sample on 3d sensor plane
              Ray r = Ray(lc, normalize(lc - spos));                                                       // construct ray

              //-- setup lens
              glm::vec3 lensP = lc;
              glm::vec3 lensN = -cam.direction;
              glm::vec3 lensX = glm::cross(lensN, glm::vec3(0, 1, 0)); // the exact vector doesnt matter
              glm::vec3 lensY = glm::cross(lensN, lensX);

              glm::vec3 lensSample = lensP + payload.rnd01.x * camera.A * lensX + payload.rnd01.y * camera.A * lensY;

              glm::vec3 focalPoint = cam.origin + (camera.S_O + S_I) * cam.direction;
              float t = glm::dot(focalPoint - r.origin, lensN) / glm::dot(r.direction, lensN);
              glm::vec3 focus = r.origin + t * r.direction;
              r = Ray(lensSample, normalize(focus - lensSample)); // TODO: Fix lens

              scene->traceRay(r, payload, 1e-4, 1e20);

              float resolver = float(params.numSamples) / float(samp + 1);
              accumulator[w + h * params.width] += payload.accumulatedRadiance / float(params.numSamples);
              image[w + h * params.width] = glm::pow(glm::max(accumulator[w + h * params.width] * resolver, 0.0f), glm::vec3(0.45f));
            }
            co_return;
          }(w, samp));
    }
    threadPool.runBatch(std::move(batch));
    auto end = std::chrono::high_resolution_clock::now();
    sampleTimes.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f);
  }
}
