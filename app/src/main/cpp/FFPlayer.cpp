//
// Created by 许成谱 on 2022/4/17.
//

#include "FFPlayer.h"


FFPlayer::FFPlayer(const char *data_source, JNICallbakcHelper *pHelper) {
    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source, data_source);
    this->helper=pHelper;
    pthread_mutex_init(&seekMutex, nullptr);
}

void *prepareTask(void *args) {
    auto *ffPlayer = static_cast<FFPlayer *>(args);
    ffPlayer->prepare_();
    return nullptr;
}

void FFPlayer::prepare() {
    //子线程中执行
    pthread_create(&pid_prepare, nullptr, prepareTask, this);
}

FFPlayer::~FFPlayer() {
    DELETE(data_source);
    DELETE(helper);
    pthread_mutex_destroy(&seekMutex);
}

void FFPlayer::prepare_() {
    //第一步：打开媒体地址（文件路径， 直播地址rtmp）
    //int avformat_open_input(AVFormatContext **ps, const char *url, ff_const59 AVInputFormat *fmt, AVDictionary **options);
    avFormatContext = avformat_alloc_context();

    AVDictionary *avDictionary = nullptr;
    av_dict_set(&avDictionary, "timeout", "5000000", 0);
    /*
     * 1，AVFormatContext *
     * 2，路径
     * 3，AVInputFormat *fmt  Mac、Windows 摄像头、麦克风， 我们目前安卓用不到
     * 4，各种设置：例如：Http 连接超时， 打开rtmp的超时  AVDictionary **options
     */
    int r = avformat_open_input(&avFormatContext, data_source, nullptr, &avDictionary);
    // 释放字典
    av_dict_free(&avDictionary);
    if (r != 0) {
        LOGD(TAG, "avformat_open_input faile");
        //打开失败回调 把错误信息反馈给Java
        if (helper){
            helper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_OPEN_URL);
        }
        //释放资源
        avformat_close_input(&avFormatContext);
        return;
    }
    //第二步：查找媒体中的音视频流的信息
    r = avformat_find_stream_info(avFormatContext, nullptr);
    if (r < 0) {
        //解析失败
        LOGD(TAG, "avformat_find_stream_info faile");
        if (helper){
            helper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        //释放资源
        avformat_close_input(&avFormatContext);
        return;
    }

    duration=avFormatContext->duration/AV_TIME_BASE;//ffmpeg里的时间都需要通过时间基转换

    AVCodecContext * avCodecContext= nullptr;

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
            //释放资源
            avformat_close_input(&avFormatContext);
            return;
        }
        //第七步：编解码器 上下文 （这个才是真正干活的）
        avCodecContext=avcodec_alloc_context3(avCodec);
        if (!avCodecContext){
            //把错误信息反馈给Java
            LOGD(TAG, "avcodec_alloc_context3 faile");
            if (helper){
                helper->onError(THREAD_CHILD,FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            //释放资源
            avcodec_free_context(&avCodecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&avFormatContext);
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
            //释放资源
            avcodec_free_context(&avCodecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&avFormatContext);
            return;
        }
        //第九步：打开解码器
       r= avcodec_open2(avCodecContext,avCodec,nullptr);
        if (r){
            //把错误信息反馈给Java
            LOGD(TAG, "avcodec_open2 faile");
            if (helper){
                helper->onError(THREAD_CHILD,FFMPEG_OPEN_DECODER_FAIL);
            }
            //释放资源
            avcodec_free_context(&avCodecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&avFormatContext);
            return;
        }
        //6.1 给timebase赋值
        AVRational timeBase= pAvStream->time_base;

        //第十步：从解码器参数中获取流的类型 视频、音频
        if (codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
            audioChannel= new AudioChannel(i,avCodecContext,timeBase);
            if (helper&&this->duration!=0){
                audioChannel->setJniCallbackHelper(helper);
            }
        }else if (codecpar->codec_type==AVMEDIA_TYPE_VIDEO){

            //6.7适配部分视频中包含封面流 封面流不能当成视频流处理，拦截
            // 虽然是视频类型，但是只有一帧封面
            if (pAvStream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                continue;
            }

            //6.2 给fps赋值
            AVRational avRational=pAvStream->avg_frame_rate;
            int fps=av_q2d(avRational);

            videoChannel=new VideoChannel(i,avCodecContext,timeBase,fps);
            videoChannel->setRenderCallback(renderCallback);

            if (helper&&this->duration!=0){
                videoChannel->setJniCallbackHelper(helper);
            }
        }
    }//循环结束
    //第十一步: 如果流中 没有音频 也没有 视频 【健壮性校验】
    if (!audioChannel&&!videoChannel){
        //把错误信息反馈给Java
        LOGD(TAG, "audioChannel is null or videoChannel is null ");
        if (helper){
            helper->onError(THREAD_CHILD,FFMPEG_NOMEDIA);
        }
        if (avCodecContext) {
            avcodec_free_context(&avCodecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
        }
        avformat_close_input(&avFormatContext);
        return;
    }
    //第十二步：走到这里说明成功了，通知Java层
    if (helper){
        helper->onPrepared(THREAD_CHILD);
    }
}

void* start_task(void* args){
    auto * ffPlayer=static_cast<FFPlayer*> (args);
    ffPlayer->start_();
    return nullptr;
}

void FFPlayer::start(){
    isplaying=1;
    if (videoChannel){
        videoChannel->start();
        videoChannel->setAudioChannel(audioChannel);
    }

    if (audioChannel){
        audioChannel->start();
    }
    //开启子线程，执行真正的开始任务，把音频和视频的压缩包加入到队列里面去
    pthread_create(&pid_start,nullptr,start_task,this);
}

void FFPlayer::start_(){
    while (isplaying){
        //5.1 限制放入队列的数量，一次性加载完内存会飙升甚至oom
        if (videoChannel&&videoChannel->packets.size()>100){
            av_usleep(10*1000);//单位：微秒  休眠10ms
            continue;
        }
        if (audioChannel&&audioChannel->packets.size()>100){
            av_usleep(10*1000);//单位：微秒  休眠10ms
            continue;
        }
        //取出压缩包放到压缩包队列里面去 可能是音频的也可能是视频的
        AVPacket * packet=av_packet_alloc();
        int ret=av_read_frame(avFormatContext,packet);
        if (!ret){
//            LOGD(TAG, "FFPlayer::start_() av_read_frame ret=%d ",ret);
            if (videoChannel&&videoChannel->stream_index==packet->stream_index){
                //插入视频的压缩包队列
                videoChannel->packets.insertToQueue(packet);
            }else if (audioChannel&&audioChannel->stream_index==packet->stream_index){
                audioChannel->packets.insertToQueue(packet);
            }
        }else if (ret==AVERROR_EOF){
            //读到文件末尾
            //注意此时是读完了，并不一定是播完了

            //5.5
            if ((videoChannel&&videoChannel->packets.empty())&&((audioChannel&&audioChannel->packets.empty()))){
                LOGD(TAG, "FFPlayer::start_() av_read_frame AVERROR_EOF ");
                break;// 队列的数据被音频 视频 全部播放完毕了，我在退出
            }
        }else{
            //av_read_frame出现错误，跳出循环
            LOGD(TAG, "FFPlayer::start_() av_read_frame 错误跳出循环 ");
            break;
        }
    }
    LOGD(TAG, "FFPlayer::start_() 结束循环 ");
    isplaying=0;
    videoChannel->stop();
    audioChannel->stop();

}

void FFPlayer::setRenderCallback(RenderCallback callback){
    renderCallback=callback;
}

int FFPlayer::getDuration() {
    return duration;
}

void FFPlayer::seek(int progress) {
    if (progress<0||progress>duration){
        //回调错误
        return;
    }
    if (audioChannel== nullptr||videoChannel== nullptr){
        //回调错误
        return;
    }
    if (avFormatContext== nullptr){
        //回调错误
        return;
    }
    // FFmpeg 大部分单位 == 时间基AV_TIME_BASE
    /**
     * 1.formatContext 安全问题
     * 2.-1 代表默认情况，FFmpeg自动选择 音频 还是 视频 做 seek，  模糊：0视频  1音频
     * 3. AVSEEK_FLAG_ANY（老实） 直接精准到 拖动的位置，问题：如果不是关键帧，B帧 可能会造成 花屏情况
     *    AVSEEK_FLAG_BACKWARD（则优  8的位置 B帧 ， 找附件的关键帧 6，如果找不到他也会花屏）
     *    AVSEEK_FLAG_FRAME 找关键帧（非常不准确，可能会跳的太多），一般不会直接用，但是会配合用
     */
    // 假设出了花屏，AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME， 缺点：慢一些
    // 有一点点冲突，后面再看 （则优  | 配合找关键帧）
    // av_seek_frame(formatContext, -1, progress * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    // 音视频正在播放，用户去 seek，我是不是应该停掉播放的数据  音频1frames 1packets，  视频1frames 1packets 队列

    //avFormatContext涉及到多线程操作，加锁
    pthread_mutex_lock(&seekMutex);
    int ret=av_seek_frame(avFormatContext, -1, progress * AV_TIME_BASE,   AVSEEK_FLAG_FRAME);
    if (ret<0){
       //回调错误
        return;
    }
    //清除数据
    if (videoChannel){
        videoChannel->packets.setWork(0);
        videoChannel->frames.setWork(0);
        videoChannel->packets.clear();
        videoChannel->frames.clear();
        videoChannel->packets.setWork(1);
        videoChannel->frames.setWork(1);
    }

    if (audioChannel){
        audioChannel->packets.setWork(0);
        audioChannel->frames.setWork(0);
        audioChannel->packets.clear();
        audioChannel->frames.clear();
        audioChannel->packets.setWork(1);
        audioChannel->frames.setWork(1);
    }
    pthread_mutex_unlock(&seekMutex);
}

void *stopTask(void* args){
    auto * ffplayer=static_cast<FFPlayer*>(args);
    if (ffplayer){
        ffplayer->stop_(ffplayer);
    }
    return nullptr; // 必须返回，坑，错误很难找
}

void FFPlayer::stop() {
    //资源释放工作
    //不回调到上层
    helper= nullptr;

    if (audioChannel) {
        audioChannel-> helper= nullptr;
    }
    if (videoChannel) {
        videoChannel->helper = nullptr;
    }
    // 如果是直接释放 我们的 prepare_ start_ 线程，不能暴力释放 ，否则会有bug

    // 让他 稳稳的停下来

    // 我们要等这两个线程 稳稳的停下来后，我再释放DerryPlayer的所以工作
    // 由于我们要等 所以会ANR异常

    // 所以我们我们在开启一个 stop_线程 来等你 稳稳的停下来
    // 创建子线程
    pthread_create(&pid_stop, nullptr,stopTask,this);
}

void FFPlayer::stop_(FFPlayer* ffPlayer){
    isplaying= false;
    //将先前开辟的子线程绑定到主线程，你释放完了我再释放
    pthread_join(pid_prepare, nullptr);
    pthread_join(pid_start, nullptr);

    if (avFormatContext){
        avformat_close_input(&avFormatContext);
        avformat_free_context(avFormatContext);
        avFormatContext= nullptr;
    }

    DELETE(audioChannel);
    DELETE(videoChannel);
    DELETE(ffPlayer);
}
