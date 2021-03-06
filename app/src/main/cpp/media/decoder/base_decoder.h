/**
  * @ProjectName:    OctoberPlayer
  * @Description:      封装解码中基础的流程
  * @Author:         qzhuorui
  * @CreateDate:     2020/10/12 16:16
 */
#ifndef OCTOBERPLAYER_BASE_DECODER_H
#define OCTOBERPLAYER_BASE_DECODER_H

#include <jni.h>
#include <string>
#include <thread>
#include "../../utils/logger.h"
#include "i_decoder.h"
#include "decode_state.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
};


class BaseDecoder : public IDecoder {

private :
    const char *TAG = "BaseDecoder";

    //-------------------------------------------FFmpeg相关的结构体参数，定义解码相关
    //解码信息上下文
    AVFormatContext *m_format_ctx = NULL;
    //解码器
    AVCodec *m_codec = NULL;
    //解码器上下文
    AVCodecContext *m_codec_ctx = NULL;
    //待解码包
    AVPacket *m_packet = NULL;
    //最终解码数据
    AVFrame *m_frame = NULL;

    //总时长
    long m_duration = 0;
    //当前播放时间
    int64_t m_cur_t_s = 0;
    //开始播放的时间
    int64_t m_started_t = -1;

    //解码状态
    DecodeState m_state = STOP;
    //数据流索引
    int m_stream_index = -1;

    //-------------------------------------------解码器基本方法
    /**
     * 初始化
     * @param env jvm环境
     * @param path 本地文件路径
     */
    void Init(JNIEnv *env, jstring path);

    /**
     * 初始化FFMpeg相关的参数
     * @param env jvm环境
     */
    void InitFFMpegDecoder(JNIEnv *env);

    /**
     * 分配解码过程中需要的缓存
     */
    void AllocFrameBuffer();

    /**
     * 循环解码
     */
    void LoopDecode();

    /**
     * 获取当前帧时间戳
     */
    void ObtainTimeStamp();

    /**
     * 解码完成
     * @param env jvm环境
     */
    void DoneDecode(JNIEnv *env);

    /**
     * 时间同步
     */
    void SyncRender();

public:
    //-------------------------------------------构造方法，析构方法
    BaseDecoder(JNIEnv *env, jstring path, bool for_synthesizer);

    virtual ~BaseDecoder();

    /**
 * 视频宽度
 * @return
 */
    int width() {
        return m_codec_ctx->width;
    }

    /**
     * 视频高度
     * @return
     */
    int height() {
        return m_codec_ctx->height;
    }

    /**
     * 视频时长
     * @return
     */
    long duration() {
        return m_duration;
    }

    //-------------------------------------------实现基类方法，解码器基础功能
    void GoOn() override;

    void Pause() override;

    void Stop() override;

    bool IsRunning() override;

    long GetDuration() override;

    long GetCurPos() override;

    //-------------------------------------------定义线程相关
    //线程依附的JVM环境，同于在新的解码线程中获取env，JNIEnv和线程是一一对应的
    JavaVM *m_jvm_for_thread = NULL;

    //原始路径jstring引用，否则无法在线程中操作
    jobject m_path_ref = NULL;
    //经过转换的路径
    const char *m_path = NULL;

    //线程等待锁变量
    pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t m_cond = PTHREAD_COND_INITIALIZER;

    // 为合成器提供解码
    bool m_for_synthesizer = false;

    /**
     * 新建解码线程
     */
    void CreateDecodeThread();

    /**
     * 静态解码方法，用于解码线程回调
     * @param that 当前解码器
     */
    static void Decode(std::shared_ptr<BaseDecoder> that);

protected:

    /**
     * 进入等待
     */
    void Wait(long second = 0);

    /**
     * 恢复解码
     */
    void SendSignal();

    /**
     * 是否为合成器提供解码
     * @return true 为合成器提供解码 false 解码播放
     */
    bool ForSynthesizer() {
        return m_for_synthesizer;
    }

    const char *path() {
        return m_path;
    }

    /**
     * 解码器上下文
     * @return
     */
    AVCodecContext *codec_cxt() {
        return m_codec_ctx;
    }

    /**
     * 视频数据编码格式
     * @return
     */
    AVPixelFormat video_pixel_format() {
        return m_codec_ctx->pix_fmt;
    }

    /**
     * 获取解码时间基
     */
    AVRational time_base() {
        return m_format_ctx->streams[m_stream_index]->time_base;
    }

    /**
     * 解码一帧数据
     * @return
     */
    AVFrame *DecodeOneFrame();

    //-------------------------------------------子类需要实现的虚函数，规定子类需要实现的方法
    /**
     * 子类准备回调方法
     * @note 注：在解码线程中回调
     * @param env 解码线程绑定的JVM环境
     */
    virtual void Prepare(JNIEnv *env) = 0;

    /**
     * 子类渲染回调方法
     * @note 注：在解码线程中回调
     * @param frame 视频：一帧YUV数据；音频：一帧PCM数据
     */
    virtual void Render(AVFrame *frame) = 0;

    /**
     * 子类释放资源回调方法
     */
    virtual void Release() = 0;

    /**
     * Log前缀
     */
    virtual const char *const LogSpec() = 0;

    /**
     * 音视频索引
     */
    virtual AVMediaType GetMediaType() = 0;

    /**
     * 是否需要自动循环解码
     */
    virtual bool NeedLoopDecode() = 0;

};


#endif //OCTOBERPLAYER_BASE_DECODER_H
