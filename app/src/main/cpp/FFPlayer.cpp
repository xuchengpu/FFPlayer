//
// Created by 许成谱 on 2022/4/17.
//

#include "FFPlayer.h"

FFPlayer::FFPlayer(const char *data_source, JNICallbakcHelper *pHelper) {
    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source, data_source);
    this->helper=pHelper;

}

void *prepareTask(void *args) {
    auto *ffPlayer = static_cast<FFPlayer *>(args);
    ffPlayer->prepare_();
    return 0;
}

void FFPlayer::prepare() {
    //子线程中执行
    pthread_create(&pid_prepare, 0, prepareTask, this);
}

FFPlayer::~FFPlayer() {
    if (data_source) {
        delete data_source;
    }
}

void FFPlayer::prepare_() {
    //第一步：打开媒体地址（文件路径， 直播地址rtmp）
    //int avformat_open_input(AVFormatContext **ps, const char *url, ff_const59 AVInputFormat *fmt, AVDictionary **options);
    avFormatContext = avformat_alloc_context();

    AVDictionary *avDictionary = 0;
    av_dict_set(&avDictionary, "timeout", "5000000", 0);
    /*
     * 1，AVFormatContext *
     * 2，路径
     * 3，AVInputFormat *fmt  Mac、Windows 摄像头、麦克风， 我们目前安卓用不到
     * 4，各种设置：例如：Http 连接超时， 打开rtmp的超时  AVDictionary **options
     */
    int r = avformat_open_input(&avFormatContext, data_source, 0, &avDictionary);
    // 释放字典
    av_dict_free(&avDictionary);
    if (r != 0) {
        LOGD(TAG, "avformat_open_input faile");
        //打开失败回调 把错误信息反馈给Java
        if (helper){
            helper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    //第二步：查找媒体中的音视频流的信息
    r = avformat_find_stream_info(avFormatContext, 0);
    if (r < 0) {
        //解析失败
        LOGD(TAG, "avformat_find_stream_info faile");
        if (helper){
            helper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }
    //第三步：根据流信息，流的个数，用循环来找
    for (int i = 0; i <avFormatContext->nb_streams ; ++i) {
        //第四步：获取媒体流（视频，音频）
        AVStream * pAvStream=avFormatContext->streams[i];
        // 第五步：从上面的流中 获取 编码解码的【参数】
        AVCodecParameters *codecpar=pAvStream->codecpar;
        //第六步：（根据上面的【参数】）获取编解码器
        AVCodec * avCodec= avcodec_find_decoder(codecpar->codec_id);
        if (!avCodec){
            if (helper){
                helper->onError(THREAD_CHILD,FFMPEG_FIND_DECODER_FAIL);
            }
        }
        //第七步：编解码器 上下文 （这个才是真正干活的）
        AVCodecContext * avCodecContext=avcodec_alloc_context3(avCodec);
        if (!avCodecContext){
            //把错误信息反馈给Java
            LOGD(TAG, "avcodec_alloc_context3 faile");
            if (helper){
                helper->onError(THREAD_CHILD,FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        //第八步：他目前是一张白纸（parameters copy codecContext）
       r= avcodec_parameters_to_context(avCodecContext,codecpar);
        if (r<0){
            //把错误信息反馈给Java
            LOGD(TAG, "avcodec_parameters_to_context faile");
            if (helper){
                helper->onError(THREAD_CHILD,FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }
        //第九步：打开解码器
       r= avcodec_open2(avCodecContext,avCodec,0);
        if (r){
            //把错误信息反馈给Java
            LOGD(TAG, "avcodec_open2 faile");
            if (helper){
                helper->onError(THREAD_CHILD,FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }
        //第十步：从解码器参数中获取流的类型 视频、音频
        if (codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
            audioChannel= new AudioChannel();
        }else if (codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
            videoChannel=new VideoChannel();
        }
    }//循环结束
    //第十一步: 如果流中 没有音频 也没有 视频 【健壮性校验】
    if (!audioChannel&&!videoChannel){
        //把错误信息反馈给Java
        LOGD(TAG, "audioChannel is null or videoChannel is null ");
        if (helper){
            helper->onError(THREAD_CHILD,FFMPEG_NOMEDIA);
        }
        return;
    }
    //第十二步：走到这里说明成功了，通知Java层
    if (helper){
        helper->onPrepared(THREAD_CHILD);
    }


}
