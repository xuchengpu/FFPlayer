//
// Created by 许成谱 on 2022/4/17.
//

#ifndef FFPLAYER_VIDEOCHANNEL_H
#define FFPLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"
class VideoChannel :public BaseChannel{

public:
    VideoChannel(int stream_index,AVCodecContext *avCodecContext);
    ~VideoChannel();

    void start();
};


#endif //FFPLAYER_VIDEOCHANNEL_H
