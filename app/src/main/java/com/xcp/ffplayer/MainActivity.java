package com.xcp.ffplayer;

import android.os.Bundle;
import android.view.SurfaceView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }
    private FFPlayer ffPlayer;
    private SurfaceView surfaceView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        surfaceView=findViewById(R.id.surfaceView);

        ffPlayer=new FFPlayer();
        //3.1 设置surfaceview到c++层
        ffPlayer.setSurfaceView(surfaceView);
        ffPlayer.setDataSource("/storage/emulated/0/Android/data/demo.mp4");
        ffPlayer.setPreparedListener(new PrepareListener() {
            @Override
            public void onPrepared() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
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
                        Toast.makeText(MainActivity.this, "onError errMsg="+errMsg, Toast.LENGTH_SHORT).show();
                    }
                });
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
}