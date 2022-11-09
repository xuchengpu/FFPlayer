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

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include <libavutil/time.h>
}
class FFPlayer {
private:
    char* data_source=0;
    pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *avFormatContext=0;
    AudioChannel *audioChannel=0;
    VideoChannel *videoChannel=0;
    JNICallbakcHelper *helper=0;
    int isplaying=0;
    RenderCallback renderCallback;
    int duration;
public:
    FFPlayer(const char *data_source, JNICallbakcHelper *pHelper);
    ~FFPlayer();
    void prepare();
    void prepare_();
    void start();
    void start_();
    void setRenderCallback(RenderCallback callback);

    int getDuration();
};


#endif //FFPLAYER_FFPLAYER_H
