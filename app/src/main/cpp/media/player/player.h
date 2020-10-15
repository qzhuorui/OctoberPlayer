//
// Created by qzr on 2020/10/13.
//播放器

#ifndef OCTOBERPLAYER_PLAYER_H
#define OCTOBERPLAYER_PLAYER_H

#include "../decoder/video/v_decoder.h"
#include "../decoder/audio/a_decoder.h"

class Player {

private:
    //视频解码器
    VideoDecoder *m_v_decoder;
    //视频渲染器
    VideoRender *m_v_render;

    AudioDecoder *m_a_decoder;
    AudioRender *m_a_render;

public:
    Player(JNIEnv *jniEnv, jstring path, jobject surface);

    ~Player();

    void player();

    void pauser();

};


#endif //OCTOBERPLAYER_PLAYER_H
