cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_STANDARD 26)

find_package(MetavisionSDK COMPONENTS core stream ui REQUIRED)

find_package(OpenCV REQUIRED)
include_Directories(${OpenCV_INCLUDE_DIRS})

find_package(spdlog REQUIRED)

find_package(OpenMP REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(main ${SOURCES})

target_include_directories(main PRIVATE /opt/homebrew/Cellar/libomp/20.1.3/include)

target_link_libraries(main PRIVATE MetavisionSDK::core MetavisionSDK::stream MetavisionSDK::ui)
target_link_libraries(main PRIVATE ${OpenCV_LIBRARIES})
target_link_libraries(main PRIVATE spdlog::spdlog)
target_link_libraries(main PRIVATE OpenMP::OpenMP_CXX)

target_compile_options(main PRIVATE -Wall)

