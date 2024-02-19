package com.example.myapplication

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import com.example.myapplication.databinding.ActivityPlayerBinding
import kotlinx.coroutines.runBlocking
import java.io.File

class PlayerActivity : AppCompatActivity() {

    private var binding: ActivityPlayerBinding? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityPlayerBinding.inflate(layoutInflater)

        setContentView(binding?.root)

        runBlocking {
            val files = File("sdcard/Pictures/screenrecorder").listFiles()

            files?.forEach {
                Log.d("DEMO","file=> ${it.name}")
            }

            if (files?.first() != null)
                Player.playWithSurface(files.first().absolutePath,binding!!.player)
        }

    }
}