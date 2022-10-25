//
// Created by 许成谱 on 2022/4/17.
//

#ifndef FFPLAYER_VIDEOCHANNEL_H
#define FFPLAYER_VIDEOCHANNEL_H
#include "BaseChannel.h"

//3.8 定义函数指针用来回调解码后的数据给window播放
typedef void (*RenderCallback)(uint8_t *, int, int, int);

class VideoChannel :public BaseChannel{
private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;

public:
    VideoChannel(int stream_index,AVCodecContext *avCodecContext);
    ~VideoChannel();

    void start();

    void stop();

    void video_decode();

    void video_play();

    void setRenderCallback(RenderCallback callback);
};


#endif //FFPLAYER_VIDEOCHANNEL_H
