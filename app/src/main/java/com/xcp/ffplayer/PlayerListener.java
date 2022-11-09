package com.xcp.ffplayer;

/**
 * Created by 许成谱 on 2022/4/15 12:33.
 * qq:1550540124
 * 热爱生活每一天！
 */
public interface PlayerListener {
    void onPrepared();
    void onError(String errMsg);
    void onProgress(int progress);
}
