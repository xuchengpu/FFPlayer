#include <jni.h>
#include "FFPlayer.h"
#include "JNICallbakcHelper.h"
#include "logging.h"
#define TAG "tag"

//
// Created by 许成谱 on 2022/4/15.
//
FFPlayer *mplayer = 0;
JavaVM *vm=0;

extern "C"
JNIEXPORT jint JNI_OnLoad(JavaVM *vm,void *args){
    ::vm=vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativePrepare(JNIEnv *env, jobject thiz, jstring data_source) {
    JNICallbakcHelper *helper=new JNICallbakcHelper(vm,env,thiz);
    const char *data_source_ = env->GetStringUTFChars(data_source, 0);
    mplayer = new FFPlayer(data_source_, helper);
    mplayer->prepare();
    env->ReleaseStringUTFChars(data_source,data_source_);
    LOGD(TAG, "Java_com_xcp_ffplayer_FFPlayer_nativePrepare");
}
extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeStart(JNIEnv *env, jobject thiz) {


}
extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeStop(JNIEnv *env, jobject thiz) {


}
extern "C"
JNIEXPORT void JNICALL
Java_com_xcp_ffplayer_FFPlayer_nativeRelease(JNIEnv *env, jobject thiz) {


}