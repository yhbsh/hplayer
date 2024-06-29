#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/time.h>

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    AVFormatContext *in_fmt_ctx   = NULL;
    AVDictionary    *in_opt       = NULL;
    AVStream        *in_stream    = NULL;
    const AVCodec   *in_codec     = NULL;
    AVCodecContext  *in_codec_ctx = NULL;
    AVPacket        *in_packet    = NULL;
    AVFrame         *in_frame     = NULL;
    int              ret          = +0;
    int              in_stream_id = -1;
    int64_t          start        = -1;

    avdevice_register_all();

    av_dict_set(&in_opt, "pixel_format", "nv12", 0);
    av_dict_set(&in_opt, "framerate", "240", 0);
    av_dict_set(&in_opt, "probesize", "32", 0);

    in_fmt_ctx   = avformat_alloc_context();
    ret          = avformat_open_input(&in_fmt_ctx, "0", av_find_input_format("avfoundation"), &in_opt);
    ret          = avformat_find_stream_info(in_fmt_ctx, NULL);
    in_codec     = NULL;
    in_stream_id = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &in_codec, 0);
    in_stream    = in_fmt_ctx->streams[in_stream_id];
    in_codec_ctx = avcodec_alloc_context3(in_codec);
    ret          = avcodec_parameters_to_context(in_codec_ctx, in_stream->codecpar);
    ret          = avcodec_open2(in_codec_ctx, in_codec, NULL);
    in_packet    = av_packet_alloc();
    in_frame     = av_frame_alloc();
    start        = av_gettime_relative();

    do {
        ret = av_read_frame(in_fmt_ctx, in_packet);

        if (ret < 0) {
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) continue;
            fprintf(stderr, "[ERROR]: av_read_frame(fc, p): %s\n", av_err2str(ret));
            break;
        }

        ret = avcodec_send_packet(in_codec_ctx, in_packet);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) continue;
            fprintf(stderr, "[ERROR]: avcodec_send_packet(cc, p): %s\n", av_err2str(ret));
            continue;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(in_codec_ctx, in_frame);
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                fprintf(stderr, "[ERROR]: avcodec_receive_frame(cc, f): %s\n", av_err2str(ret));
                continue;
            }

            int64_t pts   = av_rescale_q(in_frame->pts - in_stream->start_time, in_stream->time_base, AV_TIME_BASE_Q);
            int64_t now   = av_gettime_relative() - start;
            int64_t delay = pts - now;
            if (delay > 0) av_usleep(delay);

            printf("codec: %s | frame: %4lld | now: %9lf | pts: %9lf | delay: %9lf | format: %s\n", avcodec_get_name(in_codec->id), in_codec_ctx->frame_num, now * 1e-6, pts * 1e-6, delay * 1e-6, av_get_pix_fmt_name(in_frame->format));
        }
        av_packet_unref(in_packet);

    } while (1);

    avformat_close_input(&in_fmt_ctx);
    avcodec_free_context(&in_codec_ctx);
    av_frame_free(&in_frame);
    av_packet_free(&in_packet);

    return 0;
}
