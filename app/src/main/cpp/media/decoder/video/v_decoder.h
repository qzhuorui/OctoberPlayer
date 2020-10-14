//
// Created by qzr on 2020/10/13.
//视频播放解码器
//

#ifndef OCTOBERPLAYER_V_DECODER_H
#define OCTOBERPLAYER_V_DECODER_H

#include "../base_decoder.h"
#include "../../render/video/video_render.h"
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};


class VideoDecoder : public BaseDecoder {

private:
    const char *TAG = "VideoDecoder";

    //视频数据目标格式
    const AVPixelFormat DST_FORMAT = AV_PIX_FMT_RGBA;

    //存放YUV转化为RGB后的数据
    AVFrame *m_rgb_frame = NULL;
    uint8_t *m_buf_for_rgb_frame = NULL;

    //视频格式转换器
    SwsContext *m_sws_ctx = NULL;
    //视频渲染器
    VideoRender *m_video_render = NULL;

    //显示的目标宽
    int m_dst_w;
    //显示的目标高
    int m_dst_h;

    /**
     * 初始化渲染器
     */
    void InitRender(JNIEnv *env);

    /**
     * 初始化显示器
     * @param env
     */
    void InitBuffer();

    /**
     * 初始化视频数据转换器
     */
    void InitSws();

public:
    VideoDecoder(JNIEnv *env, jstring path, bool for_synthesizer = false);

    ~VideoDecoder();

    void SetRender(VideoRender *render);

protected:
    AVMediaType GetMediaType() override;

    //是否需要循环解码
    bool NeedLoopDecode() override;

    //准备解码环境
    void Prepare(JNIEnv *env) override;

    //渲染
    void Render(AVFrame *frame) override;

    //释放回调
    void Release() override;

    const char *const LogSpec() override;
};


#endif //OCTOBERPLAYER_V_DECODER_H
