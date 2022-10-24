//
// Created by 许成谱 on 2022/4/17.
//

#ifndef FFPLAYER_VIDEOCHANNEL_H
#define FFPLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"
class VideoChannel :public BaseChannel{
private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;

public:
    VideoChannel(int stream_index,AVCodecContext *avCodecContext);
    ~VideoChannel();

    void start();

    void stop();

    void video_decode();

    void video_play();
};


#endif //FFPLAYER_VIDEOCHANNEL_H
