//
// Created by qzr on 2020/10/15.
//

#ifndef OCTOBERPLAYER_OPENSL_RENDER_H
#define OCTOBERPLAYER_OPENSL_RENDER_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <queue>
#include <string>
#include <pthread.h>
#include "../../../utils/logger.h"
#include "audio_render.h"

extern "C" {
#include <libavutil/mem.h>
};

static const char *TAG = "OpenSLRender";

class OpenSLRender : public AudioRender {
private:

    class PcmData {
    public:
        PcmData(uint8_t *pcm, int size) {
            this->pcm = pcm;
            this->size = size;
        }

        ~PcmData() {
            if (pcm != NULL) {
                //释放已使用的内存
                free(pcm);
                pcm = NULL;
                used = false;
            }
        }

        uint8_t *pcm = NULL;
        int size = 0;
        bool used = false;
    };

    const SLuint32 SL_QUEUE_BUFFER_COUNT = 2;

    //------------------------------------定义成员变量
    //引擎对象
    SLObjectItf m_engine_obj = NULL;
    //引擎接口
    SLEngineItf m_engine = NULL;

    //混音器
    SLObjectItf m_output_mix_obj = NULL;

    //pcm播放器
    SLObjectItf m_pcm_player_obj = NULL;
    SLPlayItf m_pcm_player = NULL;
    SLVolumeItf m_pcm_player_volume = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf m_pcm_buffer;

    std::queue<PcmData *> m_data_queue;

    // 缓存线程等待锁变量
    pthread_mutex_t m_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t m_cache_cond = PTHREAD_COND_INITIALIZER;

    //------------------------------------定义相关成员方法
    //创建引擎
    bool CreateEngine();

    //创建混音器
    bool CreateOutputMixer();

    //创建播放器
    bool CreatePlayer();

    //开始播放渲染
    void StartRender();

    //音频数据压入缓冲队列
    void BlockEnqueue();

    //检查是否发生错误
    bool CheckError(SLresult result, std::string hint);

    void static sRenderPcm(OpenSLRender *that);

    //数据填充通知接口
    void static sReadPcmBufferCbFun(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context);

    void WaitForCache() {
        pthread_mutex_lock(&m_cache_mutex);
        pthread_cond_wait(&m_cache_cond, &m_cache_mutex);
        pthread_mutex_unlock(&m_cache_mutex);
    }

    void SendCacheReadySignal() {
        pthread_mutex_lock(&m_cache_mutex);
        pthread_cond_signal(&m_cache_cond);
        pthread_mutex_unlock(&m_cache_mutex);
    }

public:
    OpenSLRender();

    ~OpenSLRender();

    void InitRender() override;

    void Render(uint8_t *pcm, int size) override;

    void ReleaseRender() override;

};


#endif //OCTOBERPLAYER_OPENSL_RENDER_H
