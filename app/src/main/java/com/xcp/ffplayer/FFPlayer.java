package com.xcp.ffplayer;

import android.text.TextUtils;

/**
 * Created by 许成谱 on 2022/4/15 12:17.
 * qq:1550540124
 * 热爱生活每一天！
 * 视频播放器
 */
public class FFPlayer {

    private String dataSource;
    private PrepareListener preparedListener;

    public FFPlayer() {
    }

    public void setDataSource(String dataSource){
        this.dataSource=dataSource;
    }

    public void setPreparedListener(PrepareListener listener){
        this.preparedListener=listener;
    }

    public void prepare(){
        if(!TextUtils.isEmpty(dataSource)) {
            nativePrepare(dataSource);
        }
    }

    public void start(){
        nativeStart();
    }

    public void stop(){
        nativeStop();
    }

    public void release(){
        nativeRelease();
    }


    /**
     * 供jni反射调用
     */
    public void onPrepared(){
        if(preparedListener!=null) {
            preparedListener.onPrepared();
        }
    }
    /**
     * 供jni反射调用
     */
    public void onError(int errorCode){
        if(preparedListener!=null) {
            preparedListener.onError(errorCode);
        }
    }

    private native void nativePrepare(String dataSource);
    private native void nativeStart() ;
    private native void nativeStop() ;
    private native void nativeRelease();

}
