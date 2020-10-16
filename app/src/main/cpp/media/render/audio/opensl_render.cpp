//
// Created by qzr on 2020/10/15.
//

#include <thread>
#include <unistd.h>
#include "opensl_render.h"


//创建引擎
bool OpenSLRender::CreateEngine() {
    //1.创建引擎对象
    SLresult result = slCreateEngine(&m_engine_obj, 0, NULL, 0, NULL, NULL);
    if (CheckError(result, "Engine")) return false;

    //2,Realize初始化
    result = (*m_engine_obj)->Realize(m_engine_obj, SL_BOOLEAN_FALSE);
    if (CheckError(result, "Engine Realize")) return false;

    //3.获取引擎接口
    result = (*m_engine_obj)->GetInterface(m_engine_obj, SL_IID_ENGINE, &m_engine);
    return !CheckError(result, "Engine Interface");
}

//创建混音器
bool OpenSLRender::CreateOutputMixer() {
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    SLresult result = (*m_engine)->CreateOutputMix(m_engine, &m_output_mix_obj, 1, mids, mreq);
    if (CheckError(result, "Output Mix")) return false;

    result = (*m_output_mix_obj)->Realize(m_output_mix_obj, SL_BOOLEAN_FALSE);
    if (CheckError(result, "Output Mix Realize")) return false;

    result = (*m_output_mix_obj)->GetInterface(m_output_mix_obj, SL_IID_ENVIRONMENTALREVERB,
                                               &m_output_mix_evn_reverb);
    if (CheckError(result, "Output Mix Env Reverb")) return false;

    if (result == SL_RESULT_SUCCESS) {
        (*m_output_mix_evn_reverb)->SetEnvironmentalReverbProperties(m_output_mix_evn_reverb,
                                                                     &m_reverb_settings);
    }
    return true;
}

bool OpenSLRender::CreatePlayer() {
    //1.配置数据源DataSource
    //配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            SL_QUEUE_BUFFER_COUNT};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放PCM格式的数据
            (sl_uint32_t) 2,//2个声道（立体声）
            SL_SAMPLINGRATE_44_1,//44.1KHZ频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};//得到DataSource

    //2.配置输出DataSink
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, m_output_mix_obj};//pLocator
    SLDataSink slDataSink = {&outputMix, NULL};//得到DataSink

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    //3.创建播放器
    SLresult result = (*m_engine)->CreateAudioPlayer(m_engine, &m_pcm_player_obj, &slDataSource,
                                                     &slDataSink, 3, ids, req);
    if (CheckError(result, "Player")) return false;
    //初始化播放器
    result = (*m_pcm_player_obj)->Realize(m_pcm_player_obj, SL_BOOLEAN_FALSE);
    if (CheckError(result, "Player Realize")) return false;

    //4.获取播放器接口
    //得到接口后，获取Player接口
    result = (*m_pcm_player_obj)->GetInterface(m_pcm_player_obj, SL_IID_PLAY, &m_pcm_player);
    if (CheckError(result, "Player Volume Interface")) return false;

    //5.获取缓冲器队列接口
    //注册回调缓冲区，获取缓冲队列接口，用于将数据填入缓冲区
    result = (*m_pcm_player_obj)->GetInterface(m_pcm_player_obj, SL_IID_BUFFERQUEUE, &m_pcm_buffer);
    if (CheckError(result, "Player Queue Buffer")) return false;
    //给播放器!!!注册缓冲区接口回调，回调作用：播放器中数据播放完，会回调，通知填充新数据
    result = (*m_pcm_buffer)->RegisterCallback(m_pcm_buffer, sReadPcmBufferCbFun,
                                               this);//数据源为buffer时，需要一个缓冲接口
    //获取音量接口
    result = (*m_pcm_player_obj)->GetInterface(m_pcm_player_obj, SL_IID_VOLUME,
                                               &m_pcm_player_volume);
    if (CheckError(result, "Player Volume Interface")) return false;

    if (CheckError(result, "Register Callback Interface")) return false;

    LOGI(TAG, "OpenSL ES init success")

    return true;
}

void OpenSLRender::StartRender() {
    while (m_data_queue.empty()) {
        //数据缓冲无数据，进入等待，等待解码数据第一帧，才能开始播放!!!
        WaitForCache();
    }
    (*m_pcm_player)->SetPlayState(m_pcm_player, SL_PLAYSTATE_PLAYING);
    sReadPcmBufferCbFun(m_pcm_buffer, this);
}

//调用播放器播放接口后，往缓冲区中压入数据
void OpenSLRender::BlockEnqueue() {
    if (m_pcm_player == NULL) return;

    //先将已使用过的数据移除，回收资源
    while (!m_data_queue.empty()) {
        PcmData *pcm = m_data_queue.front();
        if (pcm->used) {
            m_data_queue.pop();
            delete pcm;
        } else {
            break;
        }
    }

    //等待数据缓冲，判断是否还有未播放的缓冲数据
    while (m_data_queue.empty() && m_pcm_player != NULL) {// if m_pcm_player is NULL, stop render
        WaitForCache();
    }

    PcmData *pcmData = m_data_queue.front();
    if (NULL != pcmData && m_pcm_player) {
        //将数据压入OpenSL队列
        SLresult result = (*m_pcm_buffer)->Enqueue(m_pcm_buffer, pcmData->pcm,
                                                   (SLuint32) pcmData->size);
        if (result == SL_RESULT_SUCCESS) {
            //只做已经使用标记，在下一帧数据压入前移除
            //保证数据能正常使用，否则可能会出现破音
            pcmData->used = true;
        }
    }
}

bool OpenSLRender::CheckError(SLresult result, std::string hint) {
    if (SL_RESULT_SUCCESS != result) {
        LOGE(TAG, "OpenSL ES [%s] init fail", hint.c_str())
        return true;
    }
    return false;
}

//StartRender()开始和在播放过程中只要OpenSL播放完数据，就会自动回调该方法，重新进入播放流程
void
OpenSLRender::sReadPcmBufferCbFun(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context) {
    OpenSLRender *player = (OpenSLRender *) context;
    player->BlockEnqueue();
}

//------

OpenSLRender::OpenSLRender() {

}

OpenSLRender::~OpenSLRender() {

}

void OpenSLRender::InitRender() {
    if (!CreateEngine()) return;
    if (!CreateOutputMixer()) return;
    if (!CreatePlayer()) return;

    //开启线程，进入播放等待。两个线程，需要等待解码数据第一帧，才能开始播放
    std::thread t(sRenderPcm, this);
    t.detach();
}

void OpenSLRender::Render(uint8_t *pcm, int size) {
    if (m_pcm_player) {
        if (pcm != NULL && size > 0) {
            //只缓存两帧数据，避免占用太多内存，导致内存申请失败，播放出现杂音
            while (m_data_queue.size() >= 2) {
                SendCacheReadySignal();
                usleep(20000);
            }

            //将数据复制一份，并压入队列
            uint8_t *data = (uint8_t *) malloc(size);
            memcpy(data, pcm, size);

            PcmData *pcmData = new PcmData(pcm, size);
            m_data_queue.push(pcmData);

            //通知播放线程退出等待，恢复播放
            SendCacheReadySignal();
        }
    } else {
        free(pcm);
    }
}

void OpenSLRender::ReleaseRender() {

}

void OpenSLRender::sRenderPcm(OpenSLRender *that) {
    that->StartRender();
}
