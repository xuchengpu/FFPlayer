package com.xcp.ffplayer;

import android.os.Bundle;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }
    private FFPlayer ffPlayer;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ffPlayer=new FFPlayer();
        ffPlayer.setDataSource("/storage/emulated/0/Android/data/com.xcp.ffplayer/cache/22.mp4");
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
            public void onError(int code) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, "onError code="+code, Toast.LENGTH_SHORT).show();
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