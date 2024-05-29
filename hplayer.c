#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <SDL2/SDL_video.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

#include <stdbool.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[Usage]: ./hplayer [file]\n");
        return 1;
    }

    int              ret    = -1;
    SDL_Window      *w      = NULL;
    SDL_Renderer    *r      = NULL;
    AVFormatContext *in_ctx = NULL;
    const AVCodec   *c      = NULL;
    AVCodecContext  *cc     = NULL;
    const AVStream  *s      = NULL;
    int              vs     = -1;
    AVFrame         *f      = NULL;
    AVPacket        *p      = NULL;

    w = SDL_CreateWindow("Hplayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800, 0);
    r = SDL_CreateRenderer(w, -1, 0);

    ret = avformat_open_input(&in_ctx, argv[1], NULL, NULL);
    ret = avformat_find_stream_info(in_ctx, NULL);
    vs  = av_find_best_stream(in_ctx, AVMEDIA_TYPE_VIDEO, 0, 0, &c, 0);
    s   = in_ctx->streams[vs];
    cc  = avcodec_alloc_context3(c);
    ret = avcodec_open2(cc, c, NULL);
    f   = av_frame_alloc();
    p   = av_packet_alloc();

    bool quit = false;
    while (!quit) {
        if (av_read_frame(in_ctx, p) < 0) quit = true;

        avcodec_send_packet(cc, p);
        while (avcodec_receive_frame(cc, f) == 0) {
            SDL_Texture *texture = SDL_CreateTexture(r, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, f->width, f->height);

            uint8_t  *yplane    = f->data[0];
            const int ylinesize = f->linesize[0];
            uint8_t  *uplane    = f->data[1];
            const int ulinesize = f->linesize[1];
            uint8_t  *vplane    = f->data[2];
            const int vlinesize = f->linesize[2];

            SDL_UpdateYUVTexture(texture, NULL, yplane, ylinesize, uplane, ulinesize, vplane, vlinesize);
            SDL_RenderCopy(r, texture, NULL, NULL);
            SDL_RenderPresent(r);
        }

        av_packet_unref(p);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
        }

        SDL_Delay(16);
    }

    av_frame_free(&f);
    av_packet_free(&p);
    avcodec_free_context(&cc);
    avformat_close_input(&in_ctx);

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);

    return 0;
}
