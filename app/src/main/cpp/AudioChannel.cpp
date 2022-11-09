//
// Created by 许成谱 on 2022/4/17.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int stream_index,AVCodecContext *avCodecContext,AVRational timeBase): BaseChannel(stream_index,avCodecContext,timeBase) {
    // 音频三要素
    /*
     * 1.采样率 44100 48000
     * 2.位声/采用格式大小  16bit == 2字节
     * 3.声道数 2  --- 人类就是两个耳朵
     */

    // 音频压缩数据包  AAC
    /**
     * 44100
     * 32bit  算法效率高 浮点运算高
     * 2
     */

    // 重采样 音频格式转换

    // 我的手机 音频参数  喇叭播放的目标
    /**
     * 44100
     * 16bit
     * 2
     */
    // 目前没有找到 32bit 的手机

    // 缓冲区大小怎么定义？ 答：out_buffers ---- out_buffers_size
    // 初始化缓冲区

    // AV_SAMPLE_FMT_S16: 位声、采用格式大小，存放大小

    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO); // STEREO:双声道类型 == 获取 声道数 2
    out_sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16); // 每个sample是16 bit == 2字节
    out_sample_rate = 44100; // 采样率

    // out_buffers_size = 176,400
    out_buffers_size = out_sample_rate * out_sample_size * out_channels; // 44100 * 2 * 2 = 176,400
    out_buffers = static_cast<uint8_t *>(malloc(out_buffers_size)); // 堆区开辟而已

    // FFmpeg 音频 重采样  音频重采样上下文 第四个
    swr_ctx  = swr_alloc_set_opts(nullptr,
                                    // 下面是输出环节
                                  AV_CH_LAYOUT_STEREO,  // 声道布局类型 双声道
                                  AV_SAMPLE_FMT_S16,  // 采样大小 16bit
                                  out_sample_rate, // 采样率  44100

                                    // 下面是输入环节
                                  avCodecContext->channel_layout, // 声道布局类型
                                  avCodecContext->sample_fmt, // 采样大小
                                  avCodecContext->sample_rate,  // 采样率
                                  0, nullptr);
    // 初始化 重采样上下文
    swr_init(swr_ctx);
}

AudioChannel::~AudioChannel() {

}

void *task_audio_decode(void *args){
    auto audioPlayer=static_cast<AudioChannel*>(args);
    audioPlayer->audio_decode();
    return nullptr;
}

void *task_audio_play(void *args){
    auto audioPlayer=static_cast<AudioChannel*>(args);
    audioPlayer->audio_play();
    return nullptr;
}

void AudioChannel::start() {
    //标记开始播放
    isPlaying = true;
    //队列开启工作模式
    packets.setWork(1);
    frames.setWork(1);
    //开启子线程，开始从packet中取出数据，将音频压缩包解码为音频原始包(PCM)
    pthread_create(&pid_audio_decode, nullptr, task_audio_decode, this);
    //开启子线程，开始从frames中取出数据，播放
    pthread_create(&pid_audio_play, nullptr, task_audio_play, this);
}

void AudioChannel::stop() {

}

void AudioChannel::audio_decode() {
    AVPacket *avPacket = nullptr;
    while (isPlaying) {
        //5.3
        if (isPlaying&&frames.size()>100){
            av_usleep(10*1000);
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
       // 音频也有帧的概念，所以获取原始包的时候，最好还是判断下【严谨性，最好是判断下】
        if (ret == AVERROR(EAGAIN)) {
            //output is not available in this state - user must try to send new input
            LOGD(TAG,TAG, "audio_decode ret == AVERROR(EAGAIN);");
            continue;
        } else if (ret != 0) {
            //出现错误，跳出循环
            //5.5
            if (avFrame){
                releaseAVFrame(&avFrame);
            }
            LOGD(TAG,TAG, "audio_decode else if (ret != 0);");
            break;
        }
        //finally we get avFrame PCM数据
        frames.insertToQueue(avFrame);

        //5.4 移除avpacket内部成员变量的堆空间
        av_packet_unref(avPacket);
        //ffmpeg会在内部拷贝一份，自己的这个可以释放掉内存
        releaseAVPacket(&avPacket);
    }
    LOGD(TAG,TAG, "audio_decode  releaseAVPacket(&avPacket);");
    av_packet_unref(avPacket);
    releaseAVPacket(&avPacket);
}

/**
 * 4.3 TODO 回调函数
 * @param bq  队列
 * @param args  this // 给回调函数的参数
 */
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void * args) {

    auto *audio_channel = static_cast<AudioChannel *>(args);

    int pcm_size = audio_channel->getPCM();

    // 添加数据到缓冲区里面去
    (*bq)->Enqueue(
            bq, // 传递自己，为什么（因为没有this，为什么没有this，因为不是C++对象，所以需要传递自己） JNI讲过了
            audio_channel->out_buffers, // PCM数据
            pcm_size); // PCM数据对应的大小，缓冲区大小怎么定义？（复杂）
}

/**
 * 1.out_buffers给予数据
 * 2.out_buffers给予数据的大小计算工作
 * @return  大小还要计算，因为我们还要做重采样工作，重采样之后，大小不同了
 */
int AudioChannel::getPCM() {
    int pcm_data_size = 0;

    // 获取PCM数据
    // PCM数据在哪里？答：队列 frames队列中  frame->data == PCM数据(待 重采样   32bit)

    AVFrame *frame = nullptr;
    while (isPlaying) {
        int ret = frames.getQueueAndDel(frame);
        if (!isPlaying) {
            break; // 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }
        if (!ret) { // ret == 0
            continue; // 哪怕是没有成功，也要继续（假设：你生产太慢(原始包加入队列)，我消费就等一下你）
        }

        // 开始重采样

        // 来源：10个48000   ---->  目标:44100  11个44100
        // 获取单通道的样本数 (计算目标样本数： ？ 10个48000 --->  48000/44100因为除不尽  11个44100)
        int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples, // 获取下一个输入样本相对于下一个输出样本将经历的延迟
                                            out_sample_rate, // 输出采样率
                                            frame->sample_rate, // 输入采样率
                                            AV_ROUND_UP); // 先上取 取去11个才能容纳的上

        // pcm的处理逻辑
        // 音频播放器的数据格式是我们自己在下面定义的
        // 而原始数据（待播放的音频pcm数据）
        // TODO 重采样工作
        // 返回的结果：每个通道输出的样本数(注意：是转换后的)    做一个简单的重采样实验(通道基本上都是:1024)
        int samples_per_channel = swr_convert(swr_ctx,
                // 下面是输出区域
                                              &out_buffers,  // 【成果的buff】  重采样后的
                                              dst_nb_samples, // 【成果的 单通道的样本数 无法与out_buffers对应，所以有下面的pcm_data_size计算】

                // 下面是输入区域
                                              (const uint8_t **) frame->data, // 队列的AVFrame * 那的  PCM数据 未重采样的
                                              frame->nb_samples); // 输入的样本数

        // 由于out_buffers 和 dst_nb_samples 无法对应，所以需要重新计算
        pcm_data_size = samples_per_channel * out_sample_size * out_channels; // 941通道样本数  *  2样本格式字节数  *  2声道数  =3764

        // 单通道样本数:1024  * 2声道  * 2(16bit)  =  4,096

        //6.3 设置每一帧的时间戳  // audio_time == 0.00000 0.0231233 0.035454 就是音频播放的时间搓
        //ffmpeg 采用了无理数的概念，用多少多少分之一的形式避免精度损失
        audioStamp=frame->best_effort_timestamp*av_q2d(timeBase);
        if(helper){
            helper->onProgress(THREAD_CHILD,audioStamp);
        }

        break; // 利用while循环 来写我们的逻辑
    } // while end

    // FFmpeg录制 Mac 麦克风  输出 每一个音频包的size == 4096
    // 4096是单声道的样本数，  44100是每秒钟采样的数
    // 采样率 和 样本数的关系？
    // 答： TODO 单通道样本数:1024  * 2声道  * 2(16bit)  =  4,096 ==  4096是单声道的样本数
    //      TODO 采样率 44100是每秒钟采样的次数

    // 样本数 = 采样率 * 声道数 * 位声

    // 双声道的样本数？  答： （采样率 * 声道数 * 位声） * 2

    //5.6
    av_frame_unref(frame); // 减1 = 0 释放成员指向的堆区
    releaseAVFrame(&frame); // 释放AVFrame * 本身的堆区空间
    return pcm_data_size;
}

void AudioChannel::audio_play() {
/**
 *  一部曲：创建引擎并获取引擎接口
    二部曲：设置混音器
    三部曲：创建播放器
    四部曲：设置播放回调函数
    五部曲：设置播放器状态为播放状态
    六部曲：手动激活回调函数
    七部曲：释放
 */
    SLresult result; // 用于接收 执行成功或者失败的返回值

    /**
     * TODO 1.创建引擎对象并获取【引擎接口】
     */

    // 1.1 创建引擎对象：SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,"创建引擎 slCreateEngine error");
        return;
    }

    // 1.2 初始化引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE); // SL_BOOLEAN_FALSE:延时等待你创建成功
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,"创建引擎 Realize error");
        return;
    }

    // 1.3 获取引擎接口
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG,"创建引擎接口 Realize error");
        return;
    }

    // 健壮判断
    if (engineInterface) {
        LOGD(TAG,"创建引擎接口 create success");
    } else {
        LOGD(TAG,"创建引擎接口 create error");
        return;
    }

    /**
     * TODO 2.设置混音器
     */
    // 2.1 创建混音器
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, nullptr, nullptr); // 环境特效，混响特效，.. 都不需要
    if (SL_RESULT_SUCCESS != result) {
        LOGD(TAG,"初始化混音器 CreateOutputMix failed");
        return;
    }

    // 2.2 初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE); // SL_BOOLEAN_FALSE:延时等待你创建成功
    if (SL_RESULT_SUCCESS != result) {
        LOGD(TAG,"初始化混音器 (*outputMixObject)->Realize failed");
        return;
    }

    // 不启用混响可以不用获取混音器接口 【声音的效果】
    // 获得混音器接口
    /*
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                             &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
    // 设置混响 ： 默认。
    SL_I3DL2_ENVIRONMENT_PRESET_ROOM: 室内
    SL_I3DL2_ENVIRONMENT_PRESET_AUDITORIUM : 礼堂 等
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
           outputMixEnvironmentalReverb, &settings);
    }
    */
    LOGI(TAG,"2、设置混音器 Success");

    /**
     * TODO 3.创建播放器
     */
    // 创建buffer缓存类型的队列  2的队列大小
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                       10};

    // pcm数据格式 == PCM是不能直接播放，mp3可以直接播放(参数集)，人家不知道PCM的参数
    //  SL_DATAFORMAT_PCM：数据格式为pcm格式
    //  2：双声道
    //  SL_SAMPLINGRATE_44_1：采样率为44100
    //  SL_PCMSAMPLEFORMAT_FIXED_16：采样格式为16bit
    // SL_PCMSAMPLEFORMAT_FIXED_16：数据大小为16bit
    // SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT：左右声道（双声道）
    // SL_BYTEORDER_LITTLEENDIAN：小端模式
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, // PCM数据格式
                                   2, // 声道数
                                   SL_SAMPLINGRATE_44_1, // 采样率（每秒44100个点）
                                   SL_PCMSAMPLEFORMAT_FIXED_16, // 每秒采样样本 存放大小 16bit
                                   SL_PCMSAMPLEFORMAT_FIXED_16, // 每个样本位数 16bit
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, // 前左声道  前右声道
                                   SL_BYTEORDER_LITTLEENDIAN}; // 字节序(小端) 例如：int类型四个字节（到底是 高位在前 还是 低位在前 的排序方式，一般我们都是小端）

    // 数据源 将上述配置信息放到这个数据源中
    // audioSrc最终配置音频信息的成果，给后面代码使用
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};
    // audioSrc 给后面代码用的
    // 独立声卡：24bit  集成声卡16bit

    // 3.2 配置音轨（输出）
    // 设置混音器
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject}; // SL_DATALOCATOR_OUTPUTMIX:输出混音器类型
    SLDataSink audioSnk = {&loc_outmix, nullptr}; // outmix最终混音器的成果，给后面代码使用
    // 需要的接口 操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

    // 3.3 创建播放器 SLObjectItf bqPlayerObject
    result = (*engineInterface)->CreateAudioPlayer(engineInterface, // 参数1：引擎接口
                                                   &bqPlayerObject, // 参数2：播放器
                                                   &audioSrc, // 参数3：音频配置信息
                                                   &audioSnk, // 参数4：混音器

            // TODO 下面代码都是 打开队列的工作
                                                   1, // 参数5：开放的参数的个数
                                                   ids,  // 参数6：代表我们需要 Buff
                                                   req // 参数7：代表我们上面的Buff 需要开放出去
    );

    if (SL_RESULT_SUCCESS != result) {
        LOGD(TAG,"创建播放器 CreateAudioPlayer failed!");
        return;
    }

    // 3.4 初始化播放器：SLObjectItf bqPlayerObject
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);  // SL_BOOLEAN_FALSE:延时等待你创建成功
    if (SL_RESULT_SUCCESS != result) {
        LOGD(TAG,"实例化播放器 CreateAudioPlayer failed!");
        return;
    }
    LOGD(TAG,"创建播放器 CreateAudioPlayer success!");

    // 3.5 获取播放器接口 【以后播放全部使用 播放器接口去干（核心）】
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay); // SL_IID_PLAY:播放接口 == iplayer
    if (SL_RESULT_SUCCESS != result) {
        LOGD(TAG,"获取播放接口 GetInterface SL_IID_PLAY failed!");
        return;
    }
    LOGI(TAG,"3、创建播放器 Success");


    /**
     * TODO 4.设置回调函数
     */
    // 4.1 获取播放器队列接口：SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue  // 播放需要的队列
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGD(TAG,"获取播放队列 GetInterface SL_IID_BUFFERQUEUE failed!");
        return;
    }

    // 4.2 设置回调 void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue,  // 传入刚刚设置好的队列
                                             bqPlayerCallback,  // 回调函数
                                             this); // 给回调函数的参数
    LOGI(TAG,"4、设置播放回调函数 Success");

    /**
     * TODO 5、设置播放器状态为播放状态
     */
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    LOGI(TAG,"5、设置播放器状态为播放状态 Success");

    // 我手动激活调用
    /**
   * TODO 6、手动激活回调函数
   */
    bqPlayerCallback(bqPlayerBufferQueue, this);
    LOGI(TAG,"6、手动激活回调函数 Success");

}
