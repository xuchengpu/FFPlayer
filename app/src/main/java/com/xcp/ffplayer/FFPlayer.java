package com.xcp.ffplayer;

import static com.xcp.ffplayer.Constants.FFMPEG_ALLOC_CODEC_CONTEXT_FAIL;
import static com.xcp.ffplayer.Constants.FFMPEG_CAN_NOT_FIND_STREAMS;
import static com.xcp.ffplayer.Constants.FFMPEG_CAN_NOT_OPEN_URL;
import static com.xcp.ffplayer.Constants.FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL;
import static com.xcp.ffplayer.Constants.FFMPEG_FIND_DECODER_FAIL;
import static com.xcp.ffplayer.Constants.FFMPEG_NOMEDIA;
import static com.xcp.ffplayer.Constants.FFMPEG_OPEN_DECODER_FAIL;

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

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    public void setPreparedListener(PrepareListener listener) {
        this.preparedListener = listener;
    }

    public void prepare() {
        if (!TextUtils.isEmpty(dataSource)) {
            nativePrepare(dataSource);
        }
    }

    public void start() {
        nativeStart();
    }

    public void stop() {
        nativeStop();
    }

    public void release() {
        nativeRelease();
    }


    /**
     * 供jni反射调用
     */
    public void onPrepared() {
        if (preparedListener != null) {
            preparedListener.onPrepared();
        }
    }

    /**
     * 供jni反射调用
     */
    public void onError(int errorCode) {
        if (preparedListener != null) {
            String msg = null;
            switch (errorCode) {
                case FFMPEG_CAN_NOT_OPEN_URL:
                    msg = "打不开视频";
                    break;
                case FFMPEG_CAN_NOT_FIND_STREAMS:
                    msg = "找不到流媒体";
                    break;
                case FFMPEG_FIND_DECODER_FAIL:
                    msg = "找不到解码器";
                    break;
                case FFMPEG_ALLOC_CODEC_CONTEXT_FAIL:
                    msg = "无法根据解码器创建上下文";
                    break;
                case FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL:
                    msg = "根据流信息 配置上下文参数失败";
                    break;
                case FFMPEG_OPEN_DECODER_FAIL:
                    msg = "打开解码器失败";
                    break;
                case FFMPEG_NOMEDIA:
                    msg = "没有音视频";
                    break;
            }
            preparedListener.onError(msg);
        }
    }

    private native void nativePrepare(String dataSource);

    private native void nativeStart();

    private native void nativeStop();

    private native void nativeRelease();

}
