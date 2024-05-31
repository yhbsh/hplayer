#include <SDL2/SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[Usage]: ./hplayer [file]\n");
        return 1;
    }

    SDL_Window      *window       = NULL;
    SDL_Renderer    *renderer     = NULL;
    SDL_Texture     *texture      = NULL;
    AVFormatContext *in_fmt_ctx   = NULL;
    const AVCodec   *in_codec     = NULL;
    AVCodecContext  *in_codec_ctx = NULL;
    const AVStream  *in_stream    = NULL;
    AVFrame         *in_frame     = NULL;
    AVPacket        *in_packet    = NULL;
    int              ret          = -1;
    int              in_stream_id = -1;

    av_log_set_level(AV_LOG_TRACE);
    ret = avformat_open_input(&in_fmt_ctx, argv[1], NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avformat_open_input(&in_ctx, argv[1], NULL, NULL): %s\n", av_err2str(ret));
        return 1;
    }
    ret = avformat_find_stream_info(in_fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avformat_find_stream_info(int_ctx, NULL): %s\n", av_err2str(ret));
        return 1;
    }
    ret = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &in_codec, 0);
    if (in_stream_id < 0) {
        fprintf(stderr, "[ERROR]: av_find_best_stream(in_ctx, AVMEDIA_TYPE_VIDEO, 0, 0, &c, 0): %s\n", av_err2str(in_stream_id));
        return 1;
    }
    in_stream_id = ret;
    in_stream    = in_fmt_ctx->streams[in_stream_id];
    in_codec_ctx = avcodec_alloc_context3(in_codec);
    ret          = avcodec_parameters_to_context(in_codec_ctx, in_stream->codecpar);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avcodec_parameters_to_context(cc, s->codecpar): %s\n", av_err2str(ret));
        return 1;
    }
    ret = avcodec_open2(in_codec_ctx, in_codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avcodec_open2(cc, c, NULL): %s\n", av_err2str(ret));
        return 1;
    }
    in_frame = av_frame_alloc();
    if (in_frame == NULL) {
        fprintf(stderr, "[ERROR]: av_frame_alloc()\n");
        return 1;
    }
    in_packet = av_packet_alloc();
    if (in_packet == NULL) {
        fprintf(stderr, "[ERROR]: av_packet_alloc()\n");
        return 1;
    }

    ret    = SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Hplayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800, 0);
    SDL_SetWindowResizable(window, SDL_TRUE);
    SDL_SetWindowBordered(window, SDL_TRUE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_bool  quit = SDL_FALSE;
    SDL_Event event;

    do {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = SDL_TRUE;
                break;
            }
        }

        ret = av_read_frame(in_fmt_ctx, in_packet);
        if (ret < 0) break;

        if (in_packet->stream_index != in_stream->index) continue;

        ret = avcodec_send_packet(in_codec_ctx, in_packet);
        if (ret < 0) {
            fprintf(stderr, "[ERROR]: avcodec_send_packet(cc, p): %s\n", av_err2str(ret));
            return 1;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(in_codec_ctx, in_frame);
            if (ret < 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                fprintf(stderr, "[ERROR]: avcodec_receive_frame(cc, f): %s\n", av_err2str(ret));
                return 1;
            }

            if (texture == NULL) texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, in_frame->width, in_frame->height);
            SDL_UpdateYUVTexture(texture, NULL, in_frame->data[0], in_frame->linesize[0], in_frame->data[1], in_frame->linesize[1], in_frame->data[2], in_frame->linesize[2]);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        av_packet_unref(in_packet);

    } while (!quit);

    av_frame_free(&in_frame);
    av_packet_free(&in_packet);
    avcodec_free_context(&in_codec_ctx);
    avformat_close_input(&in_fmt_ctx);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
