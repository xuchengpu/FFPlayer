#include <jni.h>
#include "FFPlayer.h"
#include "JNICallbakcHelper.h"

#include <android/native_window_jni.h> // ANativeWindow 用来渲染画面的 == Surface对象

//
// Created by 许成谱 on 2022/4/15.
//
FFPlayer *mPlayer = nullptr;
JavaVM *vm = nullptr;
//3.5 定义显示视频所需的window 以静态方式初始化锁变量
ANativeWindow *window = nullptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


extern "C"
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *args) {
    ::vm = vm;
    return JNI_VERSION_1_6;
}

void renderCallback(uint8_t *src_data, int width, int height, int src_lineSize) {
    LOGD(TAG, "renderCallback begin ");
    //3.10 真正执行渲染的地方
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex); // 出现了问题后，必须考虑到，释放锁，怕出现死锁问题
        return;
    }

    //把宽高信息设置到window中去
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
    //播放很简单 拿到window的缓冲区，把数据直接拷贝进去即可

    ANativeWindow_Buffer window_buffer;
    //锁定窗口的下一个绘面

    int ret = ANativeWindow_lock(window, &window_buffer, 0);
    if (ret) {
        //进来了表示发生error了，要解锁 释放资源等
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        LOGD(TAG, "ANativeWindow_lock ret error ");
        return;
    }

    uint8_t *dst = static_cast<uint8_t *> (window_buffer.bits);
    int dst_linesize = window_buffer.stride * 4;
    for (int i = 0; i < window_buffer.height; ++i) { // 图：一行一行显示 [高度不用管，用循环了，遍历高度]
        // 视频分辨率：426 * 240
        // 视频分辨率：宽 426
        // 426 * 4(rgba8888) = 1704
        // memcpy(dst_data + i * 1704, src_data + i * 1704, 1704); // 花屏
        // 花屏原因：1704 无法 64字节对齐，所以花屏

        // ANativeWindow_Buffer 64字节对齐的算法，  1704无法以64位字节对齐
        // memcpy(dst_data + i * 1792, src_data + i * 1704, 1792); // OK的
        // memcpy(dst_data + i * 1793, src_data + i * 1704, 1793); // 部分花屏，无法64字节对齐
        // memcpy(dst_data + i * 1728, src_data + i * 1704, 1728); // 花屏

        // ANativeWindow_Buffer 64字节对齐的算法  1728
        // 占位 占位 占位 占位 占位 占位 占位 占位
        // 数据 数据 数据 数据 数据 数据 数据 空值

        // ANativeWindow_Buffer 64字节对齐的算法  1792  空间换时间
        // 占位 占位 占位 占位 占位 占位 占位 占位 占位
        // 数据 数据 数据 数据 数据 数据 数据 空值 空值

        // FFmpeg为什么认为  1704 没有问题 ？
        // FFmpeg是默认采用8字节对齐的，他就认为没有问题， 但是ANativeWindow_Buffer他是64字节对齐的，就有问题
        memcpy(dst + i * dst_linesize, src_data + i * src_lineSize, dst_linesize);
//        LOGD(TAG, "memcpy window_buffer.height=%d ",window_buffer.height);
    }
    //数据刷新，显示画面
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
    LOGD(TAG, "pthread_mutex_unlock ");

}

extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativePrepare(JNIEnv *env, jobject thiz, jstring data_source) {
    JNICallbakcHelper *helper = new JNICallbakcHelper(vm, env, thiz);
    const char *data_source_ = env->GetStringUTFChars(data_source, nullptr);
    mPlayer = new FFPlayer(data_source_, helper);
    //3.7 设置播放器解码数据后传递到window的回调，用函数指针来实现
    mPlayer->setRenderCallback(renderCallback);
    mPlayer->prepare();
    env->ReleaseStringUTFChars(data_source, data_source_);
    LOGD(TAG, "Java_com_xcp_ffplayer_FFPlayer_nativePrepare");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeStart(JNIEnv *env, jobject thiz) {
    if (mPlayer) {
        mPlayer->start();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeStop(JNIEnv *env, jobject thiz) {


}
extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeRelease(JNIEnv *env, jobject thiz) {


}
extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeSetSurface(JNIEnv *env, jobject thiz, jobject surface) {
    //3.6 初始化窗口
    //涉及到多线程，使用锁来处理
    pthread_mutex_lock(&mutex);
    //释放原来的窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
    //创建新的窗口用来显示视频
    window = ANativeWindow_fromSurface(env, surface);

    pthread_mutex_unlock(&mutex);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeGetDuration(JNIEnv *env, jobject thiz) {
    if (mPlayer){
       return mPlayer->getDuration();
    }
    return 0;

}
extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeSetProgress(JNIEnv *env, jobject thiz, jint progress) {


}