cmake_minimum_required(VERSION 2.8)
project(libHKCamera_v4)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

find_package(OpenCV REQUIRED)
#find_package(FFmpeg REQUIRED)

#if (FFMPEG_FOUND)
#    message("Found FFMPEG/LibAV libraries")
#    include_directories(${FFMPEG_INCLUDE_DIR})
#else (FFMPEG_FOUND)
#    message("Can't find libavcodec, libavformat or libavutil. Add them!")
#endif (FFMPEG_FOUND)

# set(CMAKE_CXX_FLAGS "-fPIC")   #c++
# set(CMAKE_C_FLAGS "-fPIC")   #c

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include)
include_directories(/usr/local/ffmpeg_build/ffmpeg/include)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sdk/lib)
link_directories(/usr/local/lib/)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY .)

add_library(${PROJECT_NAME} SHARED CHKCamera.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME}  libavcodec.so  ${OpenCV_LIBS}  libhcnetsdk.so)
