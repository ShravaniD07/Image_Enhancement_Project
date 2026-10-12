cmake_minimum_required(VERSION 3.10)
project(Parallel_Image_Enhancement CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} src)

add_executable(seq src/sequential.cpp src/image_enhancement.cpp)
target_link_libraries(seq ${OpenCV_LIBS})

add_executable(omp src/parallel_omp.cpp src/image_enhancement.cpp)
target_link_libraries(omp ${OpenCV_LIBS} OpenMP::OpenMP_CXX)

set_target_properties(seq omp PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
