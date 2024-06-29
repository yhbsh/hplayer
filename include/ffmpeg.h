#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

int init_ffmpeg(const char *url, AVFormatContext **format_context, AVStream **video_stream, AVStream **audio_stream, AVCodecContext **video_codec_context, AVCodecContext **audio_codec_context, AVPacket **packet, AVFrame **frame);
void deinit_ffmpeg(AVFormatContext **format_context, AVCodecContext **video_codec_context, AVCodecContext **audio_codec_context, AVPacket **packet, AVFrame **frame);
