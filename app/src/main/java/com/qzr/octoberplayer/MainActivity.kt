package com.qzr.octoberplayer

import android.os.Bundle
import android.os.Environment
import android.support.v7.app.AppCompatActivity
import android.view.Surface
import android.view.SurfaceHolder
import android.view.View
import kotlinx.android.synthetic.main.activity_main.*
import java.io.File
import java.util.*

class MainActivity : AppCompatActivity() {

    private var player: Int? = null

    val path = Environment.getExternalStorageDirectory().absolutePath + "/mvtest.mp4"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        initSfv();
    }

    private fun initSfv() {
        sfv.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceChanged(
                holder: SurfaceHolder,
                format: Int,
                width: Int,
                height: Int
            ) {
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
            }

            override fun surfaceCreated(holder: SurfaceHolder) {
                if (player == null) {
                    player = createPlayer(path, holder.surface)
                    play(player!!)
                }
            }

        });
    }

    private external fun ffmpegInfo(): String

    private external fun createPlayer(path: String, surface: Surface): Int

    private external fun play(player: Int)

    private external fun pause(plyer: Int)

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

}
