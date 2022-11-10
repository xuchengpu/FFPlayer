//
// Created by 许成谱 on 2022/4/17.
//

#include "VideoChannel.h"
//6.5 定义丢包函数
/**
 * 丢压缩包
 * @param queue
 */
void dropAVPacket(queue<AVPacket *> &queue) {
    while (!queue.empty()){
        //丢弃压缩包的时候要考虑是否为I帧（关键帧）,如果丢失的是关键帧会调至P、b帧没有参考对象导致花屏
        AVPacket * avpkt=queue.front();
        if (avpkt->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::releaseAVPacket(&avpkt);
            queue.pop();
        } else{
            //跳出循环，使得只会一批只丢一个关键帧相关的帧
            break;
        }
    }
}
/**
 * 丢原始包
 * @param queue
 */
void dropAVFrame(queue<AVFrame *> &queue) {
    //已经从压缩包解压过了，不用考虑关键帧了，直接释放
    if (!queue.empty()){
        AVFrame * frame=queue.front();
        BaseChannel::releaseAVFrame(&frame);
        queue.pop();
    }
}

VideoChannel::VideoChannel(int stream_index, AVCodecContext *avCodecContext, AVRational timeBase,
                           int fps) : BaseChannel(
        stream_index, avCodecContext, timeBase), fps(fps) {
    packets.setDropDataCallback(dropAVPacket);
    frames.setDropDataCallback(dropAVFrame);
}

VideoChannel::~VideoChannel() {
    DELETE(audioChannel);
}

void *task_video_decode(void *args) {
    auto *videochannel = static_cast<VideoChannel *>(args);
    if (videochannel) {
        videochannel->video_decode();
    }
    return nullptr;
}

void *task_video_play(void *args) {
    auto *videochannel = static_cast<VideoChannel *>(args);
    if (videochannel) {
        videochannel->video_play();
    }
    return nullptr;
}

void VideoChannel::start() {
    //标记开始播放
    isPlaying = true;
    //队列开启工作模式
    packets.setWork(1);
    frames.setWork(1);
    //开启子线程，开始从packet中取出数据，将视频压缩包解码为视频原始包
    pthread_create(&pid_video_decode, nullptr, task_video_decode, this);
    //开启子线程，开始从frames中取出数据，播放
    pthread_create(&pid_video_play, nullptr, task_video_play, this);
}

void VideoChannel::stop() {


    pthread_join(pid_video_decode, nullptr);
    pthread_join(pid_video_play, nullptr);

    isPlaying= false;

    packets.setWork(0);
    frames.setWork(0);

    packets.clear();
    frames.clear();

}

void VideoChannel::video_decode() {
    AVPacket *avPacket = nullptr;
    while (isPlaying) {
        //5.2
        if (isPlaying && frames.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }

        //阻塞式函数，生产者塞入数据后就会被唤醒，告知可以消费了
        int ret = packets.getQueueAndDel(avPacket);
        //关闭播放跳出循环
        if (!isPlaying) {
            break;
        }
        //生产者生产的速度不够，等一等
        if (!ret) {
            continue;
        }
        //新版ffmpeg 解压缩数据分为两步1、发送packet 2、接收frame
        ret = avcodec_send_packet(avCodecContext, avPacket);

        if (ret) {
            //出现了错误
            break;
        }
        //从ffmpeg缓存里拿frame原始包
        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == AVERROR(EAGAIN)) {
            //output is not available in this state - user must try to send new input
            LOGD(TAG, "video_decode ret == AVERROR(EAGAIN);");
            continue;
        } else if (ret != 0) {
            //出现错误，跳出循环
            //5.5
            if (avFrame) {
                releaseAVFrame(&avFrame);
            }
            LOGD(TAG, "video_decode else if (ret != 0);");
            break;
        }
        //finally we get avFrame
        frames.insertToQueue(avFrame);

        //5.4 移除avpacket内部成员变量的堆空间
        av_packet_unref(avPacket);
        //ffmpeg会在内部拷贝一份，自己的这个可以释放掉内存
        releaseAVPacket(&avPacket);
    }
    LOGD(TAG, "video_decode  releaseAVPacket(&avPacket);");
    av_packet_unref(avPacket);
    releaseAVPacket(&avPacket);
}

void VideoChannel::video_play() {

    uint8_t *pointers[4];// RGBA
    int linesizes[4];// RGBA
    /**
     * int av_image_alloc(uint8_t *pointers[4], int linesizes[4],
                   int w, int h, enum AVPixelFormat pix_fmt, int align);
     */
    // 4、在堆内存中为pointers开辟空间
    av_image_alloc(pointers, linesizes, avCodecContext->width, avCodecContext->height,
                   AV_PIX_FMT_RGBA, 1);

    /**
     * struct SwsContext *sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat,
                                  int dstW, int dstH, enum AVPixelFormat dstFormat,
                                  int flags, SwsFilter *srcFilter,
                                  SwsFilter *dstFilter, const double *param);
     */
    //3、构造 swsContext
    SwsContext *swsContext = sws_getContext(
            //输入参数
            avCodecContext->width,
            avCodecContext->height,
            avCodecContext->pix_fmt,
            //输出参数
            avCodecContext->width,
            avCodecContext->height,
            AV_PIX_FMT_RGBA,//RGBA
            SWS_BILINEAR,//算法  该算法速度适中，画质较好
            nullptr,
            nullptr,
            nullptr
    );

    //1、从队列里拿frame原始包
    AVFrame *avFrame = nullptr;
    while (isPlaying) {
        //阻塞式函数，生产者塞入数据后就会被唤醒，告知可以消费了
        int ret = frames.getQueueAndDel(avFrame);
        //关闭播放跳出循环
        if (!isPlaying) {
            break;
        }
        //生产者生产的速度不够，等一等
        if (!ret) {
            LOGD(TAG, "video_play begin continue ");
            continue;
        }
        /**
         * int sws_scale(struct SwsContext *c, const uint8_t *const srcSlice[],
              const int srcStride[], int srcSliceY, int srcSliceH,
              uint8_t *const dst[], const int dstStride[]);
         */
        // 2、格式转换 yuv ---> rgba
        sws_scale(swsContext,
                //输入参数 YUV数据
                  avFrame->data,
                  avFrame->linesize,
                  0,
                  avCodecContext->height,
                //输出参数
                  pointers,
                  linesizes
        );

        //6.6必须在渲染之前 进行时间的判断来音视频同步 人类对音频敏感，以音频时间为基准
        //音频快  视频丢包来追上音频
        //音频慢 视频休眠来等下音频

        //视频编码之前packet包之间可能插入延迟间隔,可能没有
        double extraDelay= avFrame->repeat_pict/(2*fps);
        double fpsDelay=1.0/fps;//fps间隔  fps:每秒多少帧
        double realDelay=extraDelay+fpsDelay;

        double videoStamp=avFrame->best_effort_timestamp* av_q2d(timeBase);
        double audioStamp=audioChannel->audioStamp;
        double timeDiff=videoStamp-audioStamp;//单位秒
        LOGD(TAG, "音视频流timeDiff = %lf",timeDiff);
        if (timeDiff>0){
            //视频快了  等一等
            if (timeDiff>1){
                //时间相差过大，说明可能是拖动进度条等，不能睡眠太长时间，否则一直卡着不动会被当成bug
                av_usleep(realDelay*1000*1000);//单位转换成微秒 稍微睡会
            }else{
                av_usleep((realDelay+timeDiff)*1000*1000);//单位转换成微秒 精确的睡，把时间差拉回来
            }
        }else if (timeDiff<0){
            //视频慢于音频 赶紧通过丢包的形式追赶上音频
            //经验值 0.05
            if (fabs(timeDiff)<0.05){// fabs对负数的操作（对浮点数取绝对值）
                frames.dropData();// 多线程（安全 同步丢包）
                continue;// 丢完取下一个包
            }

        }else{
            //音视频流完全同步
            LOGD(TAG, "音视频流完全同步");
        }



        // ANatvieWindows 渲染工作
        // SurfaceView ----- ANatvieWindows
        // 如何渲染一帧图像？
        // 答：宽，高，数据  ----> 函数指针的声明
        //3.9 我拿不到Surface，只能回调给 native-lib.cpp
        if (renderCallback) {
            renderCallback(pointers[0], avCodecContext->width, avCodecContext->height,
                           linesizes[0]);
        }

        av_frame_unref(avFrame);
        releaseAVFrame(&avFrame);//已渲染完，数据包释放
    }
    //5.6
    av_frame_unref(avFrame);
    releaseAVFrame(&avFrame);//出现错误退出的循环都要释放内存
    isPlaying = false;
    av_free(&pointers[0]);
    sws_freeContext(swsContext);
}

void VideoChannel::setRenderCallback(RenderCallback callback) {
    renderCallback = callback;
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel = audioChannel;
}
