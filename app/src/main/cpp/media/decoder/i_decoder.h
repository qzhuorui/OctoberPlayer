/**
  * @ProjectName:    OctoberPlayer
  * @Description:    定义解码器的基础方法
  * @Author:         qzhuorui
  * @CreateDate:     2020/10/12 16:13
 */
#ifndef OCTOBERPLAYER_I_DECODER_H
#define OCTOBERPLAYER_I_DECODER_H

class IDecoder {
public:
    virtual void GoOn() = 0;

    virtual void Pause() = 0;

    virtual void Stop() = 0;

    virtual bool IsRunning() = 0;

    virtual long GetDuration() = 0;

    virtual long GetCurPos() = 0;
};

#endif //OCTOBERPLAYER_I_DECODER_H
