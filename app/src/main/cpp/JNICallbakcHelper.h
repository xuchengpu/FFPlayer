//
// Created by 许成谱 on 2022/4/17.
//

#ifndef FFPLAYER_JNICALLBAKCHELPER_H
#define FFPLAYER_JNICALLBAKCHELPER_H

#include "jni.h"
#include "util.h"

class JNICallbakcHelper {
private:
    JavaVM *pVm=0;
    JNIEnv *pEnv=0;
    jobject jObject;
    jmethodID mid_prepared;
    jmethodID mid_error;
    jmethodID mid_progress;
public:
    JNICallbakcHelper(JavaVM *pVm, JNIEnv *pEnv, jobject jObject);
    ~JNICallbakcHelper();
    void onPrepared(int threadMode);
    void onError(int threadMode,int errorCode);
    void onProgress(int threadMode,int progress);
};


#endif //FFPLAYER_JNICALLBAKCHELPER_H
