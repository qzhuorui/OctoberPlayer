//
// Created by qzr on 2020/10/14.
//

#include <libavutil/opt.h>
#include "a_decoder.h"
#include "../../render/audio/audio_render.h"

AudioDecoder::AudioDecoder(JNIEnv *env, const jstring path, bool for_synthesizer) : BaseDecoder(env,
                                                                                                path,
                                                                                                for_synthesizer) {
}

AudioDecoder::~AudioDecoder() {
    if (m_render != NULL) {
        delete m_render;
    }
}

void AudioDecoder::SetRender(AudioRender *render) {
    m_render = render;
}

void AudioDecoder::InitSwr() {
    //codec_ctx()为解码上下文，从子类BaseCoder中获取
    AVCodecContext *codeCtx = codec_cxt();

    //初始化格式转化工具
    m_swr = swr_alloc();

    //配置输入/输出通道类型
    av_opt_set_int(m_swr, "in_channel_layout", codeCtx->channel_layout, 0);
    av_opt_set_int(m_swr, "out_channel_layout", AUDIO_DEST_CHANNEL_LAYOUT,
                   0);//AUDIO_DEST_CHANNEL_LAYOUT立体声

    //配置输入/输出采样率
    av_opt_set_sample_fmt(m_swr, "in_sample_fmt", codeCtx->sample_fmt, 0);
    av_opt_set_sample_fmt(m_swr, "out_sample_fmt", GetSampleFmt(), 0);

    swr_init(m_swr);
}

//输出缓冲配置，分配缓冲区影响因素：采样个数，通道数，采样位数
void AudioDecoder::InitOutBuffer() {
    //重采样后一个通道采样数
    m_dest_nb_sample = (int) av_rescale_rnd(ACC_NB_SAMPLES, GetSampleRate(codec_cxt()->sample_rate),
                                            codec_cxt()->sample_rate, AV_ROUND_UP);
    //重采样后一帧数据的大小
    m_dest_data_size = (size_t) av_samples_get_buffer_size(NULL, AUDIO_DEST_CHANNEL_COUNTS,
                                                           m_dest_nb_sample, GetSampleFmt(), 1);
    m_out_buffer[0] = (uint8_t *) malloc(m_dest_data_size);
}

void AudioDecoder::InitRender() {

}

void AudioDecoder::ReleaseOutBuffer() {
    if (m_out_buffer[0] != NULL) {
        free(m_out_buffer[0]);
        m_out_buffer[0] = NULL;
    }
}

void AudioDecoder::Prepare(JNIEnv *env) {
    InitSwr();//初始化转换器
    InitOutBuffer();//初始化输出缓冲
    InitRender();//初始化渲染器
}

void AudioDecoder::Render(AVFrame *frame) {
    //转换，返回每个通道的样本数
    int ret = swr_convert(m_swr, m_out_buffer, m_dest_data_size / 2,
                          (const uint8_t **) frame->data, frame->nb_samples);//渲染之前，转换音频数据
    if (ret > 0) {
        m_render->Render(m_out_buffer[0], (size_t) m_dest_data_size);
    }
}

void AudioDecoder::Release() {
    if (m_swr != NULL) {
        swr_free(&m_swr);
    }
    if (m_render != NULL) {
        m_render->ReleaseRender();
    }
    ReleaseOutBuffer();
}



