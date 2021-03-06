/**
  * @ProjectName:    OctoberPlayer
  * @Description:      封装解码中基础的流程，实现基础解码器
  * @Author:         qzhuorui
  * @CreateDate:     2020/10/12 16:16
 */

#include "base_decoder.h"
#include "../../utils/timer.c"

//-------------------------------------------初始化解码线程
BaseDecoder::BaseDecoder(JNIEnv *env, jstring path, bool for_synthesizer)
        : m_for_synthesizer(for_synthesizer) {
    Init(env, path);
    CreateDecodeThread();
}

BaseDecoder::~BaseDecoder() {
    if (m_format_ctx != NULL) delete m_format_ctx;
    if (m_codec_ctx != NULL) delete m_codec_ctx;
    if (m_frame != NULL) delete m_frame;
    if (m_packet != NULL) delete m_packet;
}

void BaseDecoder::Init(JNIEnv *env, jstring path) {
    m_path_ref = env->NewGlobalRef(path);
    m_path = env->GetStringUTFChars(path, NULL);
    //获取JVM虚拟机，为创建线程做准备
    env->GetJavaVM(&m_jvm_for_thread);
}

void BaseDecoder::CreateDecodeThread() {
    //使用智能指针，线程结束自动删除本类指针
    std::shared_ptr<BaseDecoder> that(this);
    //开启线程
    std::thread t(Decode, that);
    t.detach();
}

//-------------------------------------------封装解码流程
//Decode方法在线程回调中
void BaseDecoder::Decode(std::shared_ptr<BaseDecoder> that) {
    //本线程对应的JNIEnv
    JNIEnv *env;

    //将线程附加到虚拟机，并获取env
    if (that->m_jvm_for_thread->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOG_ERROR(that->TAG, that->LogSpec(), "Fail to Init decode thread");
        return;
    }

    //初始化解码器
    that->InitFFMpegDecoder(env);
    //分配解码帧数据内存
    that->AllocFrameBuffer();
    //virtual,回调子类方法，通知子类解码器初始化完毕，进行初始化渲染
    that->Prepare(env);
    //进入解码循环
    that->LoopDecode();
    //退出解码
    that->DoneDecode(env);

    //解除线程和JVM关联
    that->m_jvm_for_thread->DetachCurrentThread();
}

//初始化-》打开解码器
void BaseDecoder::InitFFMpegDecoder(JNIEnv *env) {
    //1.初始化上下文
    m_format_ctx = avformat_alloc_context();
    //2.打开文件
    int error_code = avformat_open_input(&m_format_ctx, m_path, NULL, NULL);
    if (error_code != 0) {
        LOG_ERROR(TAG, LogSpec(), "Fail to open file resultCode [%d]", error_code);
        DoneDecode(env);
        return;
    }
    //3.获取音视频流信息
    if (avformat_find_stream_info(m_format_ctx, NULL) < 0) {
        LOG_ERROR(TAG, LogSpec(), "Fail to find stream info");
        DoneDecode(env);
        return;
    }

    //4.查找编解码器
    //4.1获取视频流索引
    int vIdx = -1;
    for (int i = 0; i < m_format_ctx->nb_streams; ++i) {
        if (m_format_ctx->streams[i]->codecpar->codec_type == GetMediaType()) {
            vIdx = i;
            break;
        }
    }
    if (vIdx == -1) {
        LOG_ERROR(TAG, LogSpec(), "Fail to find stream index")
        DoneDecode(env);
        return;
    }
    m_stream_index = vIdx;//数据流索引
    //4.2获取解码器参数
    AVCodecParameters *codecPar = m_format_ctx->streams[vIdx]->codecpar;
    //4.3获取解码器
    m_codec = avcodec_find_decoder(codecPar->codec_id);
    //4.4获取解码器上下文
    m_codec_ctx = avcodec_alloc_context3(m_codec);
    if (avcodec_parameters_to_context(m_codec_ctx, codecPar) != 0) {
        LOG_ERROR(TAG, LogSpec(), "Fail to obtain av codec context");
        DoneDecode(env);
        return;
    }

    //5.打开解码器
    if (avcodec_open2(m_codec_ctx, m_codec, NULL) < 0) {
        LOG_ERROR(TAG, LogSpec(), "Fail to open av codec");
        DoneDecode(env);
        return;
    }

    m_duration = (long) ((float) m_format_ctx->duration / AV_TIME_BASE * 1000);

    LOG_INFO(TAG, LogSpec(), "Decoder init success");
}

//初始化待解码和解码数据结构，仅分配内存,m_packet,m_frame
void BaseDecoder::AllocFrameBuffer() {
    //1.初始化AVPacket，存放解码前的数据
    m_packet = av_packet_alloc();
    //2.初始化AVFrame，存放解码后的数据
    m_frame = av_frame_alloc();
}

//循环解码
void BaseDecoder::LoopDecode() {
    if (STOP == m_state) {
        //如果已被外部改变状态，维持外部配置
        m_state = START;
    }
    LOG_INFO(TAG, LogSpec(), "Start loop decode");

    while (1) {
        if (m_state != DECODING && m_state != START && m_state != STOP) {
            Wait();
            //恢复同步起始时间，去除等待流逝的时间，可以理解为新的起点参照点；音视频同步方案
            m_started_t = GetCurMsTime() - m_cur_t_s;
        }

        if (m_state == STOP) {
            break;
        }

        if (-1 == m_started_t) {
            m_started_t = GetCurMsTime();
        }

        if (DecodeOneFrame() != NULL) {
            //时间同步
            SyncRender();
            Render(m_frame);//子类去实现

            if (m_state == START) {
                m_state = PAUSE;
            }
        } else {
            LOG_INFO(TAG, LogSpec(), "m_state = %d", m_state);
            if (ForSynthesizer()) {
                m_state = STOP;
            } else {
                m_state = FINISH;
            }
        }
    }
}

//解码一帧数据，交由子类渲染处理
AVFrame *BaseDecoder::DecodeOneFrame() {
    //1.从m_format_ctx读取一帧解封好的待解码数据，存放m_packet
    int ret = av_read_frame(m_format_ctx, m_packet);
    while (ret == 0) {
        if (m_packet->stream_index == m_stream_index) {
            //2.将m_packet发送到解码器中解码，解码好的数据存放在m_codec_ctx中
            switch (avcodec_send_packet(m_codec_ctx, m_packet)) {
                case AVERROR_EOF: {
                    av_packet_unref(m_packet);
                    LOG_ERROR(TAG, LogSpec(), "Decode error: %s", av_err2str(AVERROR_EOF));
                    return NULL; //解码结束
                }
                case AVERROR(EAGAIN): {
                    LOG_ERROR(TAG, LogSpec(), "Decode error: %s", av_err2str(AVERROR(EAGAIN)));
                    break;
                }
                case AVERROR(ENOMEM):
                    LOG_ERROR(TAG, LogSpec(), "Decode error: %s", av_err2str(AVERROR(ENOMEM)));
                    break;
                default:
                    break;
            }
            //3.接收一帧解码好的数据，存放在m_frame中
            int result = avcodec_receive_frame(m_codec_ctx, m_frame);
            if (result == 0) {
                ObtainTimeStamp();//获取当前帧时间戳
                av_packet_unref(m_packet);
                return m_frame;
            } else {
                LOG_INFO(TAG, LogSpec(), "Receive frame error result: %d",
                         av_err2str(AVERROR(result)))
            }
        }
        //释放packet，否则会内存泄漏
        av_packet_unref(m_packet);
        ret = av_read_frame(m_format_ctx, m_packet);
    }
    av_packet_unref(m_packet);
    LOGI(TAG, "ret = %d", ret)
    return NULL;
}

//解码完毕，释放资源
void BaseDecoder::DoneDecode(JNIEnv *env) {
    LOG_INFO(TAG, LogSpec(), "Decode done and decoder release");
    //释放缓存
    if (m_packet != NULL) {
        av_packet_free(&m_packet);
    }
    if (m_frame != NULL) {
        av_frame_free(&m_frame);
    }
    //关闭解码器
    if (m_codec_ctx != NULL) {
        avcodec_close(m_codec_ctx);
        avcodec_free_context(&m_codec_ctx);
    }
    //关闭输入流
    if (m_format_ctx != NULL) {
        avformat_close_input(&m_format_ctx);
        avformat_free_context(m_format_ctx);
    }
    //释放转换参数
    if (m_path_ref != NULL && m_path != NULL) {
        env->ReleaseStringUTFChars((jstring) m_path_ref, m_path);
        env->DeleteGlobalRef(m_path_ref);
    }
    //通知子类释放资源
    Release();
}


void BaseDecoder::Wait(long second) {
    pthread_mutex_lock(&m_mutex);
    if (second > 0) {
        timeval now;
        timespec outtime;
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + second;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_cond_timedwait(&m_cond, &m_mutex, &outtime);
    } else {
        pthread_cond_wait(&m_cond, &m_mutex);
    }
    pthread_mutex_unlock(&m_mutex);
}

void BaseDecoder::SendSignal() {
    pthread_mutex_lock(&m_mutex);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

void BaseDecoder::ObtainTimeStamp() {
    if (m_frame->pkt_dts != AV_NOPTS_VALUE) {
        m_cur_t_s = m_packet->dts;
    } else if (m_frame->pts != AV_NOPTS_VALUE) {
        m_cur_t_s = m_frame->pts;
    } else {
        m_cur_t_s = 0;
    }
    m_cur_t_s = (int64_t) ((m_cur_t_s * av_q2d(m_format_ctx->streams[m_stream_index]->time_base)) *
                           1000);
}

void BaseDecoder::SyncRender() {
    int64_t ct = GetCurMsTime();
    int64_t passTime = ct - m_started_t;
    if (m_cur_t_s > passTime) {
        av_usleep((unsigned int) ((m_cur_t_s - passTime) * 1000));
    }
}

void BaseDecoder::GoOn() {
    m_state = DECODING;
    SendSignal();
}

void BaseDecoder::Pause() {
    m_state = PAUSE;
}

void BaseDecoder::Stop() {
    m_state = STOP;
    SendSignal();
}

bool BaseDecoder::IsRunning() {
    return DECODING == m_state;
}

long BaseDecoder::GetDuration() {
    return m_duration;
}

long BaseDecoder::GetCurPos() {
    return (long) m_cur_t_s;
}
