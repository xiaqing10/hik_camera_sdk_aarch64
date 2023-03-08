cmake_minimum_required(VERSION 2.8)
project(plate)

find_package(OpenCV REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk/include)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk/lib)

ADD_EXECUTABLE(test main.cpp)
target_link_libraries(test ${OpenCV_LIBS} libhcnetsdk.so libpthread.so)
