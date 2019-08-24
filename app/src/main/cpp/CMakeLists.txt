cmake_minimum_required(VERSION 3.4.1)


file(GLOB SOURCE *.cpp)
add_library(
        musicplayer
        SHARED
        ${SOURCE})
find_library(
        log-lib
        log)

include_directories(src/main/cpp/include)
set(my_lib_path "../../../../libs/${ANDROID_ABI}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${my_lib_path}")

target_link_libraries(
        musicplayer
        avfilter avformat avcodec avutil swresample swscale
        ${log-lib}
        android
        z
        OpenSLES

)