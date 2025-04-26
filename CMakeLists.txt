cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_STANDARD 26)

find_package(MetavisionSDK COMPONENTS core stream ui REQUIRED)

find_package(OpenCV REQUIRED)
include_Directories(${OpenCV_INCLUDE_DIRS})

find_package(spdlog REQUIRED)


include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_library(lib SHARED ${SOURCES})

target_link_libraries(lib PRIVATE MetavisionSDK::core MetavisionSDK::stream MetavisionSDK::ui)
target_link_libraries(lib PRIVATE ${OpenCV_LIBRARIES})
target_link_libraries(lib PRIVATE spdlog::spdlog)

target_compile_options(lib PRIVATE -Wall)

