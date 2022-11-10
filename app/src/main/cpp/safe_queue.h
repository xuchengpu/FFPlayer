//
// Created by 许成谱 on 2022/10/20.
//
#ifndef FFPLAYER_SAFE_QUEUE_H
#define FFPLAYER_SAFE_QUEUE_H

#include <queue>
#include <pthread.h>
#include "logging.h"

#define TAG "ffplayer-lib"

using namespace std;

/*
 * 安全队列，采用生产消费者模式，取不到就阻塞住，直到有数据进来或者停止工作
 */
template<typename T>
class SafeQueue {
private:
    typedef void (*ReleaseCallback)(T *);

    //6.4 设置丢包的函数，同样以回调的形式交由外界去释放
    typedef void (*DropDataCallback)(queue<T> &);

private:
    queue<T> queue;
    pthread_mutex_t mutex;//互斥锁 安全
    pthread_cond_t cond;//等待和唤醒
    int work;//标记队列是否工作
    ReleaseCallback releaseCallback;
    DropDataCallback dropDataCallback;
public:
    SafeQueue() {
        pthread_mutex_init(&mutex, 0);
        pthread_cond_init(&cond, 0);
    }

    ~ SafeQueue() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    /*
     * 加入队列
     * AVPacket* 压缩包、 AVFrame *原始包
     */
    void insertToQueue(T value) {
        pthread_mutex_lock(&mutex);//多线程的访问加锁
        if (work) {
            //队列工作中……
            queue.push(value);//插入数据包
            pthread_cond_signal(&cond);//插入数据包后，发出信号，唤醒当前条件下的线程，通知去取
//            LOGD(TAG, "insertToQueue ");
        } else {
            //队列没有工作，回调到外界通知释放资源
            if (releaseCallback) {
                //由于T类型不明确，这里没有办法直接释放，所以定义一个函数指针，回调给外界，让外界自己释放
                releaseCallback(&value);
            }
        }
        pthread_mutex_unlock(&mutex);//多线程访问，释放锁
    }

    /*
     * 出队
     * AVPacket* 压缩包、 AVFrame *原始包
     */
    int getQueueAndDel(T &value) {
        pthread_mutex_lock(&mutex);//多线程操作加锁
        while (work && queue.empty()) {
            pthread_cond_wait(&cond, &mutex);//没有数据就阻塞在这里，直到根据条件变量被唤醒
        }
        if (!queue.empty()) {
            value = queue.front();
            queue.pop();//从队列中移除出去
//            LOGD(TAG, "getQueueAndDel ");
            pthread_mutex_unlock(&mutex);
            return 1;//1表示true;
        }
        pthread_mutex_unlock(&mutex);
        return 0;//0 表示false;
    }

    /**
     * 设置队列是否工作，用于类似于在activity的ondestroy中释放资源等
     * @param work 1：工作 0：不工作
     *
     */
    void setWork(int work) {
        pthread_mutex_lock(&mutex);
        this->work = work;
        //唤醒一下，因为此时队列有可能在取的时候处于阻塞状态
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    bool empty() {
        return queue.empty();
    }

    int size() {
        return queue.size();
    }

    /**
     * 清空队列中的数据，一个一个循环删除
     */
    void clear() {
        pthread_mutex_lock(&mutex);
        unsigned int size = queue.size();
        for (int i = 0; i < size; i++) {
            //循环释放队列中的数据
            T value = queue.front();
            if (releaseCallback) {
                releaseCallback(&value);
            }
            queue.pop();
        }
        pthread_mutex_unlock(&mutex);
    }

    /**
     * 设置回调函数，用于释放资源
     * @param callback
     */
    void setReleaseCallback(ReleaseCallback callback) {
        this->releaseCallback = callback;
    }

    void setDropDataCallback(DropDataCallback callback) {
        this->dropDataCallback = callback;
    }

    void dropData() {
        //操作的变量涉及到多线程，必须使用锁
        pthread_mutex_lock(&mutex);
        dropDataCallback(queue); // 函数指针 具体丢包动作，让外界完成
        pthread_mutex_unlock(&mutex);
    }
};


#endif //FFPLAYER_SAFE_QUEUE_H
