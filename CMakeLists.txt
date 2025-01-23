cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
set(CMAKE_INSTALL_MESSAGE LAZY)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake)

project(RayTracer)


find_package(assimp CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)

add_executable(RayTracer "")
target_include_directories(RayTracer PUBLIC src/)
target_link_libraries(RayTracer PUBLIC assimp::assimp)
target_link_libraries(RayTracer PUBLIC glfw)
target_link_libraries(RayTracer PUBLIC glm::glm)
target_link_libraries(RayTracer PUBLIC KTX::ktx)

add_subdirectory(src/)