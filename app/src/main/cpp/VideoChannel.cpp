//
// Created by 许成谱 on 2022/4/17.
//

#include "VideoChannel.h"

VideoChannel::VideoChannel(int stream_index, AVCodecContext *avCodecContext) : BaseChannel(
        stream_index, avCodecContext) {

}

VideoChannel::~VideoChannel() {

}

void *task_video_decode(void *args) {
    auto *videochannel = static_cast<VideoChannel *>(args);
    videochannel->video_decode();
    return 0;
}

void *task_video_play(void *args) {
    auto *videochannel = static_cast<VideoChannel *>(args);
    videochannel->video_play();
    return 0;
}

void VideoChannel::start() {
    //标记开始播放
    isPlaying = true;
    //队列开启工作模式
    packets.setWork(1);
    frames.setWork(1);
    //开启子线程，开始从packet中取出数据，将视频压缩包解码为视频原始包
    pthread_create(&pid_video_decode, 0, task_video_decode, this);
    //开启子线程，开始从frames中取出数据，播放
    pthread_create(&pid_video_play, 0, task_video_play, this);
}

void VideoChannel::stop() {

}

void VideoChannel::video_decode() {
    AVPacket *avPacket = 0;
    while (isPlaying) {
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
        //ffmpeg会在内部拷贝一份，自己的这个可以释放掉内存
        releaseAVPacket(&avPacket);
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
            LOGD(TAG, "video_decode else if (ret != 0);");
            break;
        }
        //finally we get avFrame
        frames.insertToQueue(avFrame);
    }
    LOGD(TAG, "video_decode  releaseAVPacket(&avPacket);");
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
            NULL,
            NULL,
            NULL
    );

    //1、从队列里拿frame原始包
    AVFrame *avFrame = 0;
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
        // ANatvieWindows 渲染工作
        // SurfaceView ----- ANatvieWindows
        // 如何渲染一帧图像？
        // 答：宽，高，数据  ----> 函数指针的声明
        //3.9 我拿不到Surface，只能回调给 native-lib.cpp
        if (renderCallback){
            renderCallback(pointers[0],avCodecContext->width,avCodecContext->height,linesizes[0]);
        }
        releaseAVFrame(&avFrame);//已渲染完，数据包释放
    }
    releaseAVFrame(&avFrame);//出现错误退出的循环都要释放内存
    isPlaying= false;
    av_free(&pointers[0]);
    sws_freeContext(swsContext);
}

void VideoChannel::setRenderCallback(RenderCallback callback){
    renderCallback=callback;
}
