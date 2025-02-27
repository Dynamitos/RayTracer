cmake_minimum_required(VERSION 3.27)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
set(CMAKE_INSTALL_MESSAGE LAZY)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake)

project(RayTracer)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(RayTracer "")
target_include_directories(RayTracer PUBLIC src/)
target_link_libraries(RayTracer PUBLIC Vulkan::Vulkan)
target_link_libraries(RayTracer PUBLIC Vulkan::Headers)
target_link_libraries(RayTracer PUBLIC GPUOpen::VulkanMemoryAllocator)
target_link_libraries(RayTracer PUBLIC assimp::assimp)
target_link_libraries(RayTracer PUBLIC glfw)
target_link_libraries(RayTracer PUBLIC imgui::imgui)
target_link_libraries(RayTracer PUBLIC GLEW::GLEW)
target_link_libraries(RayTracer PUBLIC glm::glm)
target_link_libraries(RayTracer PUBLIC KTX::ktx)
if(WIN32)
target_include_directories(RayTracer PUBLIC ${VCPKG_INSTALLED_DIR}/x64-windows/include)
target_link_libraries(RayTracer PUBLIC ${VCPKG_INSTALLED_DIR}/x64-windows/lib/slang.lib)
elseif(APPLE)
target_include_directories(RayTracer PUBLIC ${VCPKG_INSTALLED_DIR}/arm64-osx/include)
SET(CMAKE_OSX_DEPLOYMENT_TARGET 15.0)
target_link_libraries(RayTracer PUBLIC
  "-framework Metal"
  "-framework MetalKit"
  "-framework AppKit"
  "-framework Foundation"
  "-framework QuartzCore"
)

endif()
add_subdirectory(src/)