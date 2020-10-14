//
// Created by qzr on 2020/10/13.
//视频播放编解码器

#include "v_decoder.h"

VideoDecoder::VideoDecoder(JNIEnv *env, jstring path, bool for_synthesizer)
        : BaseDecoder(env, path, for_synthesizer) {
}

VideoDecoder::~VideoDecoder() {

}

void VideoDecoder::SetRender(VideoRender *render) {
    this->m_video_render = render;
}

void VideoDecoder::InitRender(JNIEnv *env) {
    if (m_video_render != NULL) {
        int dst_size[2] = {-1, -1};
        m_video_render->InitRender(env, width(), height(), dst_size);

        m_dst_w = dst_size[0];
        m_dst_h = dst_size[1];

        if (m_dst_w == -1) {
            m_dst_w = width();
        }
        if (m_dst_h == -1) {
            m_dst_h = height();
        }
        LOGI(TAG, "dst %d, %d", m_dst_w, m_dst_h);
    } else {
        LOGE(TAG, "Init render error, you should call SetRender first!");
    }
}

void VideoDecoder::InitBuffer() {
    m_rgb_frame = av_frame_alloc();//初始化但没分配缓存内存
    //获取缓存大小
    int numBytes = av_image_get_buffer_size(DST_FORMAT, m_dst_w, m_dst_h, 1);//计算所需内存块大小
    //分配内存
    m_buf_for_rgb_frame = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));//分配内存
    //将内存分配给RgbFrame，并将内存格式化为三个通道后，分别保存其地址
    av_image_fill_arrays(m_rgb_frame->data, m_rgb_frame->linesize,
                         m_buf_for_rgb_frame, DST_FORMAT, m_dst_w, m_dst_h, 1);//内存给到AVFrame
}

void VideoDecoder::InitSws() {
    //初始化格式转换工具：将原画面数据和目标画面数据的长宽，格式等传递进去
    m_sws_ctx = sws_getContext(width(), height(), video_pixel_format(), m_dst_w, m_dst_h,
                               DST_FORMAT, SWS_FAST_BILINEAR, NULL, NULL, NULL);
}

bool VideoDecoder::NeedLoopDecode() {
    return false;
}

//初始化，准备解码环境，初始化完解码器之后调用
void VideoDecoder::Prepare(JNIEnv *env) {
    InitRender(env);//初始化渲染器
    InitBuffer();//存放数据缓存初始化
    InitSws();//初始化数据转换工具
}

void VideoDecoder::Render(AVFrame *frame) {
    sws_scale(m_sws_ctx, frame->data, frame->linesize, 0,
              height(), m_rgb_frame->data, m_rgb_frame->linesize);
    //自定义类，封装一帧数据的相关内容
    OneFrame *one_frame = new OneFrame(m_rgb_frame->data[0], m_rgb_frame->linesize[0], frame->pts,
                                       time_base(), NULL,
                                       false);
    m_video_render->Render(one_frame);
}

void VideoDecoder::Release() {
    LOGE(TAG, "[VIDEO] release");
    if (m_rgb_frame != NULL) {
        av_frame_free(&m_rgb_frame);
        m_rgb_frame = NULL;
    }
    if (m_buf_for_rgb_frame != NULL) {
        free(m_buf_for_rgb_frame);
        m_buf_for_rgb_frame = NULL;
    }
    if (m_sws_ctx != NULL) {
        sws_freeContext(m_sws_ctx);
        m_sws_ctx = NULL;
    }
    if (m_video_render != NULL) {
        m_video_render->ReleaseRender();
        m_video_render = NULL;
    }
}

const char *const VideoDecoder::LogSpec() {
    return "VIDEO";
}

AVMediaType VideoDecoder::GetMediaType() {
    return AVMEDIA_TYPE_VIDEO;
}
