#include <SDL2/SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[Usage]: ./hplayer [file]\n");
        return 1;
    }

    int              ret    = -1;
    SDL_Window      *w      = NULL;
    SDL_Renderer    *r      = NULL;
    SDL_Texture     *t      = NULL;
    AVFormatContext *in_ctx = NULL;
    const AVCodec   *c      = NULL;
    AVCodecContext  *cc     = NULL;
    const AVStream  *s      = NULL;
    int              vs     = -1;
    AVFrame         *f      = NULL;
    AVPacket        *p      = NULL;

    ret = avformat_open_input(&in_ctx, argv[1], NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avformat_open_input(&in_ctx, argv[1], NULL, NULL): %s\n", av_err2str(ret));
        return 1;
    }
    ret = avformat_find_stream_info(in_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avformat_find_stream_info(int_ctx, NULL): %s\n", av_err2str(ret));
        return 1;
    }
    vs = av_find_best_stream(in_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &c, 0);
    if (vs < 0) {
        fprintf(stderr, "[ERROR]: av_find_best_stream(in_ctx, AVMEDIA_TYPE_VIDEO, 0, 0, &c, 0): %s\n", av_err2str(vs));
        return 1;
    }
    s   = in_ctx->streams[vs];
    cc  = avcodec_alloc_context3(c);
    ret = avcodec_parameters_to_context(cc, s->codecpar);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avcodec_parameters_to_context(cc, s->codecpar): %s\n", av_err2str(ret));
        return 1;
    }
    ret = avcodec_open2(cc, c, NULL);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avcodec_open2(cc, c, NULL): %s\n", av_err2str(ret));
        return 1;
    }
    f = av_frame_alloc();
    if (f == NULL) {
        fprintf(stderr, "[ERROR]: av_frame_alloc()\n");
        return 1;
    }
    p = av_packet_alloc();
    if (p == NULL) {
        fprintf(stderr, "[ERROR]: av_packet_alloc()\n");
        return 1;
    }

    ret = SDL_Init(SDL_INIT_VIDEO);
    w   = SDL_CreateWindow("Hplayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800, 0);
    SDL_SetWindowResizable(w, SDL_TRUE);
    SDL_SetWindowBordered(w, SDL_TRUE);
    r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);

    int       quit = 0;
    SDL_Event e;

    do {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = 1;
                break;
            }
        }

        ret = av_read_frame(in_ctx, p);
        if (ret < 0) break;

        if (p->stream_index != s->index) continue;

        ret = avcodec_send_packet(cc, p);
        if (ret < 0) {
            fprintf(stderr, "[ERROR]: avcodec_send_packet(cc, p): %s\n", av_err2str(ret));
            return 1;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(cc, f);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) {
                fprintf(stderr, "[ERROR]: avcodec_receive_frame(cc, f): %s\n", av_err2str(ret));
                return 1;
            }

            if (t == NULL) {
                t = SDL_CreateTexture(r, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, f->width, f->height);
            }
            SDL_UpdateYUVTexture(t, NULL, f->data[0], f->linesize[0], f->data[1], f->linesize[1], f->data[2], f->linesize[2]);
            SDL_RenderClear(r);
            SDL_RenderCopy(r, t, NULL, NULL);
            SDL_RenderPresent(r);
        }

        av_packet_unref(p);

    } while (!quit);

    av_frame_free(&f);
    av_packet_free(&p);
    avcodec_free_context(&cc);
    avformat_close_input(&in_ctx);

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);

    return 0;
}
