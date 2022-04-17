cmake_minimum_required(VERSION 3.6.4111459 )

SET(FFMPEG ${CMAKE_SOURCE_DIR}/ffmpeg)#ffmpeg路径
SET(RTMP ${CMAKE_SOURCE_DIR}/rtmp)#rtmp路径

#导入ffmpeg头文件
include_directories(${FFMPEG}/include)
#指定库地址，系统会自动去这个地址下寻找目标库
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${FFMPEG}/libs/${CMAKE_ANDROID_ARCH_ABI}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${RTMP}/libs/${CMAKE_ANDROID_ARCH_ABI}") # rtmp库指定

#批量导入源文件
file(GLOB srcFiles *.cpp)

add_library(native-lib #生成的库名称
        SHARED #动态库
        ${srcFiles}) #源文件

target_link_libraries(
        native-lib #总库libnative-lib.so
        # 忽略顺序的方式，导入
        -Wl,--start-group
        avcodec avfilter avformat avutil swresample swscale
        -Wl,--end-group

        log # 日志库，打印日志用的
        z # libz.so库，是FFmpeg需要用ndk的z库，FFMpeg需要额外支持  libz.so
        rtmp # rtmp 后面会专门介绍
        android # android 后面会专门介绍，目前先要明白的是 ANativeWindow 用来渲染画面的
        OpenSLES # OpenSLES 后面会专门介绍，目前先要明白的是 OpenSLES 用来播放声音的
)
