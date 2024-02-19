package com.example.myapplication

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import com.example.myapplication.databinding.ActivityPlayerBinding
import kotlinx.coroutines.runBlocking
import java.io.File

class PlayerActivity : AppCompatActivity() {

    private var binding: ActivityPlayerBinding? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityPlayerBinding.inflate(layoutInflater)

        setContentView(binding?.root)

        binding?.player?.holder?.addCallback(object : SurfaceHolder.Callback{
            override fun surfaceCreated(p0: SurfaceHolder) {
                Player.surfaceCreated(p0.surface)
                binding?.startBtn?.isEnabled = true
            }

            override fun surfaceChanged(p0: SurfaceHolder, p1: Int, p2: Int, p3: Int) {

            }

            override fun surfaceDestroyed(p0: SurfaceHolder) {
                binding?.startBtn?.isEnabled = false
            }
        })

        binding?.startBtn?.setOnClickListener {
            val files = File("sdcard/Pictures/screenrecorder").listFiles()
            if (files?.first() != null)
                Player.playWithSurface(files.first().absolutePath)
        }
    }
}