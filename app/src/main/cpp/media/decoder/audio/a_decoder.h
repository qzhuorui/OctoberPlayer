//
// Created by qzr on 2020/10/14.
//

#ifndef OCTOBERPLAYER_A_DECODER_H
#define OCTOBERPLAYER_A_DECODER_H


#include <libswresample/swresample.h>
#include "../base_decoder.h"
#include "../../../utils/const.h"
#include "../../render/audio/audio_render.h"

class AudioDecoder : public BaseDecoder {

private:
    const char *TAG = "AudioDecoder";

    //音频转换器，用来转化采样率，解码通道数，采样位数
    //这里用来将音频参数转换为双通道立体声，统一采样位数
    SwrContext *m_swr = NULL;
    //音频渲染器
    AudioRender *m_render = NULL;

    //输出缓冲
    uint8_t *m_out_buffer[1] = {NULL};

    //重采样后，每个通道包含的采样数
    //aac默认为1024，重采样后可能会变化
    int m_dest_nb_sample = 1024;

    //重采样后，一帧数据的大小
    size_t m_dest_data_size = 0;

    //初始化转换工具
    void InitSwr();

    //初始化输出缓冲
    void InitOutBuffer();

    //初始化渲染器
    void InitRender();

    //释放缓冲区
    void ReleaseOutBuffer();

    //------设置/统一 采样格式和采样率
    //采样格式：16位，设置个默认值
    AVSampleFormat GetSampleFmt() {
        return AV_SAMPLE_FMT_S16;
    }

    //目标采样率，设置个默认值
    int GetSampleRate(int spr) {
        return AUDIO_DEST_SAMPLE_RATE;
    }

public:
    AudioDecoder(JNIEnv *env, const jstring path, bool for_synthesizer);

    ~AudioDecoder();

    void SetRender(AudioRender *render);

protected:
    void Prepare(JNIEnv *env) override;

    void Render(AVFrame *frame) override;

    void Release() override;

    bool NeedLoopDecode() override {
        return true;
    }

    AVMediaType GetMediaType() override {
        return AVMEDIA_TYPE_AUDIO;
    }

    const char *const LogSpec() override {
        return "AUDIO";
    }

};


#endif //OCTOBERPLAYER_A_DECODER_H
