//
// Created by qzr on 2020/10/15.
//音频渲染接口

#ifndef OCTOBERPLAYER_AUDIO_RENDER_H
#define OCTOBERPLAYER_AUDIO_RENDER_H

#include <stdint.h>
#include <jni.h>

class AudioRender {
public:
    virtual void InitRender() = 0;

    virtual void Render(uint8_t *pcm, int size) = 0;

    virtual void ReleaseRender() = 0;

    virtual ~AudioRender() {};
};

#endif //OCTOBERPLAYER_AUDIO_RENDER_H
