//
// Created by qzr on 2020/10/13.
//

#include "player.h"
#include "../render/video/native_render/native_render.h"

Player::Player(JNIEnv *jniEnv, jstring path, jobject surface) {
    m_v_decoder = new VideoDecoder(jniEnv, path);
    m_v_render = new NativeRender(jniEnv, surface);
    m_v_decoder->SetRender(m_v_render);
}

Player::~Player() {
//不需要delete成员指针
//在BaseDecoder中的线程已经使用智能指针，会自动释放
}

void Player::player() {
    if (m_v_decoder != NULL) {
        m_v_decoder->GoOn();
    }
}

void Player::pauser() {
    if (m_v_decoder != NULL) {
        m_v_decoder->Pause();
    }
}
