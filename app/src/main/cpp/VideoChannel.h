//
// Created by 许成谱 on 2022/4/17.
//

#ifndef FFPLAYER_VIDEOCHANNEL_H
#define FFPLAYER_VIDEOCHANNEL_H
#include "BaseChannel.h"
#include "AudioChannel.h"

//3.8 定义函数指针用来回调解码后的数据给window播放
typedef void (*RenderCallback)(uint8_t *, int, int, int);

class VideoChannel :public BaseChannel{
private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;

    //6.2
    int fps;//一秒钟播放多少帧
    AudioChannel *audioChannel= nullptr;//这个主要是用于从这里获取音频流对应的frame的时间戳

public:
    VideoChannel(int stream_index,AVCodecContext *avCodecContext,AVRational timeBase,int fps);
    ~VideoChannel();

    void start();

    void stop();

    void video_decode();

    void video_play();

    void setRenderCallback(RenderCallback callback);

    //6.3 设置音频的引用
    void setAudioChannel(AudioChannel* audioChannel );
};


#endif //FFPLAYER_VIDEOCHANNEL_H
