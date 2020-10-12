/**
  * @ProjectName:    OctoberPlayer
  * @Description:    定义解码器状态
  * @Author:         qzhuorui
  * @CreateDate:     2020/10/12 16:11
 */
#ifndef OCTOBERPLAYER_DECODE_STATE_H
#define OCTOBERPLAYER_DECODE_STATE_H

enum DecodeState {
    STOP,
    PREPARE,
    START,
    DECODING,
    PAUSE,
    FINISH
};
#endif //OCTOBERPLAYER_DECODE_STATE_H
