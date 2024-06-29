#include "ffmpeg.h"

int init_ffmpeg(const char *url, AVFormatContext **format_context, AVStream **video_stream, AVStream **audio_stream, AVCodecContext **video_codec_context, AVCodecContext **audio_codec_context, AVPacket **packet, AVFrame **frame) {
    //av_log_set_level(AV_LOG_DEBUG);

    int ret = avformat_open_input(format_context, url, NULL, NULL);
    if (ret < 0) { return ret; }

    ret = avformat_find_stream_info(*format_context, NULL);
    if (ret < 0) { return ret; }

    const AVCodec *video_codec = NULL;
    const AVCodec *audio_codec = NULL;

    ret = av_find_best_stream(*format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    if (ret >= 0) {
        *video_stream = (*format_context)->streams[ret];
        *video_codec_context = avcodec_alloc_context3(video_codec);
        if (*video_codec_context == NULL) { return AVERROR(ENOMEM); }
        ret = avcodec_parameters_to_context(*video_codec_context, (*video_stream)->codecpar);
        if (ret < 0) { return ret; }
        ret = avcodec_open2(*video_codec_context, video_codec, NULL);
        if (ret < 0) { return ret; }
    }

    ret = av_find_best_stream(*format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
    if (ret >= 0) {
        *audio_stream = (*format_context)->streams[ret];
        *audio_codec_context = avcodec_alloc_context3(audio_codec);
        if (*audio_codec_context == NULL) { return AVERROR(ENOMEM); }
        ret = avcodec_parameters_to_context(*audio_codec_context, (*audio_stream)->codecpar);
        if (ret < 0) { return ret; }
        ret = avcodec_open2(*audio_codec_context, audio_codec, NULL);
        if (ret < 0) { return ret; }
    }

    *packet = av_packet_alloc();
    if (*packet == NULL) { return AVERROR(ENOMEM); }

    *frame = av_frame_alloc();
    if (*frame == NULL) { return AVERROR(ENOMEM); }

    return 0;
}

void deinit_ffmpeg(AVFormatContext **format_context, AVCodecContext **video_codec_context, AVCodecContext **audio_codec_context, AVPacket **packet, AVFrame **frame) {
    av_packet_free(packet);
    av_frame_free(frame);
    avcodec_free_context(video_codec_context);
    avcodec_free_context(audio_codec_context);
    avformat_close_input(format_context);
}
