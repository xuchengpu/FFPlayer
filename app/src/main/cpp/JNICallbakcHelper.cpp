//
// Created by 许成谱 on 2022/4/17.
//

#include "JNICallbakcHelper.h"

JNICallbakcHelper::JNICallbakcHelper(JavaVM *pVm, JNIEnv *pEnv, jobject jObject) : pVm(pVm),
                                                                                          pEnv(pEnv),
                                                                                          jObject(jObject) {
    // this->job = job; // 坑： jobject不能跨越线程，不能跨越函数，必须全局引用

    this->jObject=pEnv->NewGlobalRef(jObject);

    jclass  ffPlayerclass=pEnv->GetObjectClass(jObject);
    mid_prepared=pEnv->GetMethodID(ffPlayerclass,"onPrepared","()V");
    mid_error=pEnv->GetMethodID(ffPlayerclass,"onError","(I)V");
}
JNICallbakcHelper::~JNICallbakcHelper() {

    pVm=0;
    pEnv->DeleteGlobalRef(jObject);
    jObject =0;
    pEnv=0;

}

void JNICallbakcHelper::onPrepared(int threadMode){
    if (threadMode==THREAD_MAIN){
        pEnv->CallVoidMethod(this->jObject,mid_prepared);
    }else if (threadMode==THREAD_CHILD){
        //子线程要先attached到jvm上
        // 子线程 env也不可以跨线程吧 对的   全新的env
        JNIEnv *env_child;
        this->pVm->AttachCurrentThread(&env_child,0);
        env_child->CallVoidMethod(this->jObject,mid_prepared);
        this->pVm->DetachCurrentThread();
    }
}

void JNICallbakcHelper::onError(int threadMode,int errorCode) {
    if (threadMode==THREAD_MAIN){
        pEnv->CallVoidMethod(this->jObject,mid_error,errorCode);
    }else if (threadMode==THREAD_CHILD){
        //子线程要先attached到jvm上
        // 子线程 env也不可以跨线程吧 对的   全新的env
        JNIEnv *env_child;
        this->pVm->AttachCurrentThread(&env_child,0);
        env_child->CallVoidMethod(this->jObject,mid_error,errorCode);
        this->pVm->DetachCurrentThread();
    }
}

