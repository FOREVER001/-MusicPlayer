package com.tianzhuan.musicplayer;

public class WangyiPlayer {
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("musicplayer");
    }

    //输入input.mp3 输出out.pcm
    public native void sound(String inpiut,String output);
}
