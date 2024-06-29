#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define URL "rtsp://localhost:554/stream"

int main(void) {
    AVFormatContext    *in_fmt_ctx    = NULL;
    const AVCodec      *in_codec      = NULL;
    AVCodecContext     *in_codec_ctx  = NULL;
    AVStream           *in_stream     = NULL;
    AVDictionary       *in_opt        = NULL;
    AVPacket           *in_packet     = NULL;
    AVFrame            *in_frame      = NULL;
    AVFormatContext    *out_fmt_ctx   = NULL;
    const AVCodec      *out_codec     = NULL;
    AVCodecContext     *out_codec_ctx = NULL;
    AVStream           *out_stream    = NULL;
    AVFrame            *out_frame     = NULL;
    struct SwsContext  *sws_ctx       = NULL;
    int32_t             ret           = -1;
    int32_t             in_stream_id  = -1;
    int64_t             start         = -1;
    int64_t             next_pts      = +0;

    av_log_set_level(AV_LOG_TRACE);
    avdevice_register_all();
    avformat_network_init();

    av_dict_set(&in_opt, "pixel_format", "nv12", 0);
    av_dict_set(&in_opt, "framerate", "144", 0);
    av_dict_set(&in_opt, "probesize", "32", 0);

    in_fmt_ctx                   = avformat_alloc_context();
    ret                          = avformat_open_input(&in_fmt_ctx, "0", av_find_input_format("avfoundation"), &in_opt);
    ret                          = avformat_find_stream_info(in_fmt_ctx, NULL);
    in_stream_id                 = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &in_codec, 0);
    in_stream                    = in_fmt_ctx->streams[in_stream_id];
    in_codec_ctx                 = avcodec_alloc_context3(in_codec);
    ret                          = avcodec_parameters_to_context(in_codec_ctx, in_stream->codecpar);
    ret                          = avcodec_open2(in_codec_ctx, in_codec, NULL);
    ret                          = avformat_alloc_output_context2(&out_fmt_ctx, NULL, "rtsp", URL);
    out_codec                    = avcodec_find_encoder_by_name("h264_videotoolbox");
    out_codec_ctx                = avcodec_alloc_context3(out_codec);
    out_stream                   = avformat_new_stream(out_fmt_ctx, out_codec);
    out_codec_ctx->bit_rate      = 1000000;
    out_codec_ctx->width         = in_codec_ctx->width;
    out_codec_ctx->height        = in_codec_ctx->height;
    out_codec_ctx->time_base     = (AVRational){1, 24};
    out_codec_ctx->framerate     = (AVRational){24, 1};
    out_codec_ctx->color_range   = AVCOL_RANGE_MPEG;
    out_codec_ctx->gop_size      = 50;
    out_codec_ctx->max_b_frames  = 2;
    out_codec_ctx->pix_fmt       = AV_PIX_FMT_YUV420P;
    out_codec_ctx->flags         = out_codec_ctx->flags | AV_CODEC_FLAG_GLOBAL_HEADER;
    ret                          = avcodec_open2(out_codec_ctx, out_codec, NULL);
    ret                          = avcodec_parameters_from_context(out_stream->codecpar, out_codec_ctx);
    ret                          = avio_open(&out_fmt_ctx->pb, URL, AVIO_FLAG_WRITE);
    ret                          = avformat_write_header(out_fmt_ctx, NULL);
    in_packet                    = av_packet_alloc();
    in_frame                     = av_frame_alloc();
    out_frame                    = av_frame_alloc();
    out_frame->format            = out_codec_ctx->pix_fmt;
    out_frame->width             = out_codec_ctx->width;
    out_frame->height            = out_codec_ctx->height;
    ret                          = av_frame_get_buffer(out_frame, 0);
    sws_ctx                      = sws_getContext(in_codec_ctx->width, in_codec_ctx->height, in_codec_ctx->pix_fmt, out_codec_ctx->width, out_codec_ctx->height, out_codec_ctx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
    start                        = av_gettime();

    printf("in_cocdec_ctx->pix_fmt: %s - out_codec_ctx: %s\n", av_get_pix_fmt_name(in_codec_ctx->pix_fmt), av_get_pix_fmt_name(out_codec_ctx->pix_fmt));

    uint8_t ch;
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(STDIN_FILENO, F_SETFL, flags) == -1) {
        perror("fcntl");
        exit(1);
    }
    while (1) {
        if (read(STDIN_FILENO, &ch, 1) > 0) {
            if (ch == 'q' || ch == 'Q') {
                fprintf(stderr, "Exiting on user command.\n");
                goto clean;
            }
        }
        ret = av_read_frame(in_fmt_ctx, in_packet);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) continue;
            fprintf(stderr, "[ERROR]: av_read_frame(fmt_ctx, in_packet): %s\n", av_err2str(ret));
            goto fail;
        }

        ret = avcodec_send_packet(in_codec_ctx, in_packet);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) continue;
            fprintf(stderr, "[ERROR]: avcodec_send_packet(in_codec_ctx, in_packet): %s\n", av_err2str(ret));
            goto fail;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(in_codec_ctx, in_frame);
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                fprintf(stderr, "[ERROR]: avcodec_receive_frame(in_codec_ctx, in_frame): %s\n", av_err2str(ret));
                goto fail;
            }

            sws_scale(sws_ctx, (const uint8_t *const *) in_frame->data, in_frame->linesize, 0, in_codec_ctx->height, out_frame->data, out_frame->linesize);
            out_frame->pts = next_pts++;

            ret = avcodec_send_frame(out_codec_ctx, out_frame);
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                fprintf(stderr, "[ERROR]: avcodec_receive_frame(in_codec_ctx, in_frame): %s\n", av_err2str(ret));
                goto fail;
            }

            AVPacket *out_packet = av_packet_alloc();
            while (ret >= 0) {
                ret = avcodec_receive_packet(out_codec_ctx, out_packet);
                if (ret < 0) {
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                    fprintf(stderr, "[ERROR]: avcodec_receive_frame(in_codec_ctx, in_frame): %s\n", av_err2str(ret));
                    goto fail;
                }

                av_packet_rescale_ts(out_packet, out_codec_ctx->time_base, out_stream->time_base);
                out_packet->stream_index = out_stream->index;

                int64_t pts_time = av_rescale_q(out_packet->dts, out_stream->time_base, AV_TIME_BASE_Q);
                int64_t now_time = av_gettime() - start;
                if (pts_time > now_time) av_usleep(pts_time - now_time);

                av_interleaved_write_frame(out_fmt_ctx, out_packet);
                av_packet_unref(out_packet);
            }
        }

        av_packet_unref(in_packet);
    }

fail:
    fprintf(stderr, "An error occurred\n");
    goto clean;

clean:
    av_write_trailer(out_fmt_ctx);
    avformat_close_input(&in_fmt_ctx);
    avio_closep(&out_fmt_ctx->pb);
    avformat_free_context(out_fmt_ctx);
    avcodec_free_context(&in_codec_ctx);
    avcodec_free_context(&out_codec_ctx);
    av_frame_free(&in_frame);
    av_frame_free(&out_frame);
    av_packet_free(&in_packet);
    sws_freeContext(sws_ctx);

    avformat_network_deinit();

    return 0;
}
