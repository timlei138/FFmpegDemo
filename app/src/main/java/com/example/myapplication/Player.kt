package com.example.myapplication

import android.view.Surface
import android.view.SurfaceView

object Player {

    init {

        System.loadLibrary("ffmpeg-study")
        init()
    }


    external fun init()

    external fun getVersion(): String

    external fun playWithSurface(path: String): Int

    external fun surfaceCreated(surface: Surface)



}