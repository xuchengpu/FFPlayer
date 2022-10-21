//
// Created by 许成谱 on 2022/4/17.
//

#ifndef FFPLAYER_AUDIOCHANNEL_H
#define FFPLAYER_AUDIOCHANNEL_H
#include "BaseChannel.h"

class AudioChannel :public BaseChannel{

public:
    AudioChannel(int stream_index,AVCodecContext *avCodecContext );
    ~AudioChannel();

    void start();
};


#endif //FFPLAYER_AUDIOCHANNEL_H
