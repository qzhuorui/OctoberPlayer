/**
  * @ProjectName:    OctoberPlayer
  * @Description:      封装解码中基础的流程
  * @Author:         qzhuorui
  * @CreateDate:     2020/10/12 16:16
 */
#include "base_decoder.h"
#include "../../utils/timer.c"

//-------------------------------------------初始化解码线程
BaseDecoder::BaseDecoder(JNIEnv *env, jstring path) {
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
    std::shared_ptr <BaseDecoder> that(this);
    std::thread t(Decode, that);
    t.detach();
}

//-------------------------------------------封装解码流程
void BaseDecoder::Decode(std::shared_ptr <BaseDecoder> that) {
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
    //回调子类方法，通知子类解码器初始化完毕
    that->Prepare(env);
    //进入解码循环
    that->LoopDecode();
    //退出解码
    that->DoneDecode(env);

    //解除线程和JVM关联
    that->m_jvm_for_thread->DetachCurrentThread();

}
