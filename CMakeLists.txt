cmake_minimum_required(VERSION 3.25)
project(MyNewProject CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(FetchContent)

find_package(MetavisionSDK COMPONENTS core stream ui REQUIRED)
find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)
find_package(TBB REQUIRED)
find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include_directories(include)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(imgui)

file(GLOB_RECURSE APP_SOURCES "src/*.cpp")

file(GLOB IMGUI_CORE "imgui/*.cpp")

add_executable(main
    ${APP_SOURCES}
    ${IMGUI_CORE}
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_link_libraries(main PRIVATE
    MetavisionSDK::core
    MetavisionSDK::stream
    MetavisionSDK::ui
    ${OpenCV_LIBRARIES}
    spdlog::spdlog
    TBB::tbb
    glfw
    OpenGL::GL
)

target_compile_features(main PRIVATE cxx_std_20)
target_compile_options(main PRIVATE -Wall -fpermissive)
