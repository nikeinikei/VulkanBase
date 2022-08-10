cmake_minimum_required(VERSION 3.10)
project(VulkanTest)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

file(GLOB IMGUI_SOURCES imgui/ *.cpp)

add_executable(VulkanTest
        src/main.cpp
        ${IMGUI_SOURCES})

target_link_libraries(VulkanTest 
    ${Vulkan_LIBRARIES}
    glfw)

include_directories(${Vulkan_INCLUDE_DIRS} imgui)
