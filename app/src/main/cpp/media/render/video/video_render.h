//
// Created by qzr on 2020/10/13.
//定义视频渲染器

#ifndef OCTOBERPLAYER_VIDEO_RENDER_H
#define OCTOBERPLAYER_VIDEO_RENDER_H

#include <stdint.h>
#include <jni.h>

#include "../../one_frame.h"

class VideoRender {
public:
    virtual void InitRender(JNIEnv *env, int video_width, int video_height, int *dst_size) = 0;

    virtual void Render(OneFrame *one_frame) = 0;

    virtual void ReleaseRender() = 0;
};

#endif //OCTOBERPLAYER_VIDEO_RENDER_H
