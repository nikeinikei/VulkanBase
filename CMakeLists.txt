cmake_minimum_required(VERSION 3.10)
project(VulkanTest)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(dependencies/glfw-3.3.8)

find_package(Vulkan REQUIRED)

file(GLOB IMGUI_SOURCES dependencies/imgui/ *.cpp)

add_executable(VulkanTest
        src/main.cpp
        ${IMGUI_SOURCES})

target_link_libraries(VulkanTest 
        ${Vulkan_LIBRARIES}
        glfw)

include_directories(
        ${Vulkan_INCLUDE_DIRS} 
        dependencies/imgui
        dependencies/glm)

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VulkanTest)
endif()
