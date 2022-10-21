//
// Created by 许成谱 on 2022/4/17.
//

#include "VideoChannel.h"

VideoChannel::VideoChannel(int stream_index,AVCodecContext *avCodecContext): BaseChannel(stream_index,avCodecContext) {

}
VideoChannel::~VideoChannel() {

}

void VideoChannel::start() {

}
