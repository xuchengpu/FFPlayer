//
// Created by 许成谱 on 2022/10/21.
//

#ifndef FFPLAYER_BASECHANNEL_H
#define FFPLAYER_BASECHANNEL_H

#include "safe_queue.h"
#include "logging.h"
#include "JNICallbakcHelper.h"


extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#define TAG "ffplayer-lib"

class BaseChannel {
public:
    int stream_index;//音频或者视频的下标
    SafeQueue<AVPacket *> packets;//压缩的数据包
    SafeQueue<AVFrame *> frames;//原始的数据包
    AVCodecContext *avCodecContext = 0;//音频、视频解码器上下文
    bool isPlaying;//是否播放
    //6.1 播放流的时间基准,音视频流都有，音频流和视频流同步用
    AVRational timeBase;
    JNICallbakcHelper *helper=0;

    BaseChannel(int stream_index, AVCodecContext *avCodecContext,AVRational timeBase)
            : stream_index(stream_index), avCodecContext(avCodecContext), timeBase(timeBase) {
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);

    }

    //父类析构一定要加virtual
    virtual ~BaseChannel() {
        packets.clear();
        frames.clear();
    }

    void setJniCallbackHelper(JNICallbakcHelper *pHelper) {
        this->helper=pHelper;
    }

    /**
     * 释放队列中的所有 AVPacket*
     * @param pPacket
     */
    static void releaseAVPacket(AVPacket **pPacket) {
        if (pPacket) {
            av_packet_free(pPacket);
            *pPacket = 0;
        }
    }

    /**
     * 释放队列中的所有 AVFrame*
     * @param pPacket
     */
    static void releaseAVFrame(AVFrame **pFrame) {
        if (pFrame) {
            av_frame_free(pFrame);
            *pFrame = 0;
        }
    }
};


#endif //FFPLAYER_BASECHANNEL_H
