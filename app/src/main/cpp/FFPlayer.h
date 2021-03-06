//
// Created by 许成谱 on 2022/4/17.
//

#ifndef FFPLAYER_FFPLAYER_H
#define FFPLAYER_FFPLAYER_H


#include "jni.h"
#include "cstring"
#include "pthread.h"
#include "logging.h"
#include "VideoChannel.h"
#include "AudioChannel.h"
#include "JNICallbakcHelper.h"
#include "util.h"
#include "logging.h"
#define TAG "tag"
extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
}
class FFPlayer {
private:
    char* data_source=0;
    pthread_t pid_prepare;
    AVFormatContext *avFormatContext=0;
    AudioChannel *audioChannel=0;
    VideoChannel *videoChannel=0;
    JNICallbakcHelper *helper=0;
public:
    FFPlayer(const char *data_source, JNICallbakcHelper *pHelper);
    ~FFPlayer();
    void prepare();
    void prepare_();

};


#endif //FFPLAYER_FFPLAYER_H
