package com.xcp.ffplayer;

import static com.xcp.ffplayer.Util.getMinutes;
import static com.xcp.ffplayer.Util.getSeconds;

import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity implements SeekBar.OnSeekBarChangeListener {

    static {
        System.loadLibrary("native-lib");
    }

    private FFPlayer ffPlayer;
    private SurfaceView surfaceView;
    private TextView tvTime;
    private SeekBar seekBar;
    private int duration;
    private LinearLayout llSeekbar;
    private boolean isOnTouch;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        surfaceView = findViewById(R.id.surfaceView);
        llSeekbar = findViewById(R.id.ll_seekbar);

        tvTime = findViewById(R.id.tv_time);
        seekBar = findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(this);
        seekBar.setMax(100);

        ffPlayer = new FFPlayer();
        //3.1 设置surfaceview到c++层
        ffPlayer.setSurfaceView(surfaceView);
        ffPlayer.setDataSource("/storage/emulated/0/Android/data/demo.mp4");
//        ffPlayer.setDataSource("/storage/emulated/0/Android/data/22.mp4");
//        ffPlayer.setDataSource("http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear2/prog_index.m3u8");
//        ffPlayer.setDataSource("http://vfx.mtime.cn/Video/2019/02/04/mp4/190204084208765161.mp4");
        ffPlayer.setPlayerListener(new PlayerListener() {
            @Override
            public void onPrepared() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        duration = ffPlayer.getDuration();
                        if (duration > 0) {//0：在线视频 ,>0说明是本地文件
                            //显示播放进度条
                            llSeekbar.setVisibility(View.VISIBLE);
                            tvTime.setText("00:00/" + getMinutes(duration) + ":" + getSeconds(duration));
                        } else {
                            llSeekbar.setVisibility(View.GONE);
                        }
                        Toast.makeText(MainActivity.this, "准备完毕", Toast.LENGTH_SHORT).show();
                    }
                });
                ffPlayer.start();
            }

            @Override
            public void onError(String errMsg) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, "onError errMsg=" + errMsg, Toast.LENGTH_SHORT).show();
                    }
                });
            }

            @Override
            public void onProgress(int progress) {
                //底层播放进度的回调
                if (!isOnTouch) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            // progress:C++层 ffmpeg获取的当前播放【时间（单位是秒 80秒都有，肯定不符合界面的显示） -> 1分20秒】
                            tvTime.setText(getMinutes(progress) + ":" + getSeconds(progress)
                                    + "/" +
                                    getMinutes(duration) + ":" + getSeconds(duration));
                            //进度条动起来
                            seekBar.setProgress(progress*100/duration);
                        }
                    });
                }
            }
        });
    }

    @Override
    protected void onStart() {
        super.onStart();
        ffPlayer.prepare();
    }

    @Override
    protected void onStop() {
        super.onStop();
        ffPlayer.stop();
    }

    @Override
    protected void onDestroy() {
        ffPlayer.release();
        super.onDestroy();
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if(fromUser) {
            tvTime.setText(getMinutes(progress * duration / 100)
                    + ":" +
                    getSeconds(progress * duration / 100) + "/" +
                    getMinutes(duration) + ":" + getSeconds(duration));
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        isOnTouch = true;
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        isOnTouch = false;
        int progress = seekBar.getProgress();
        ffPlayer.setProgress(progress*duration/100);
    }
}