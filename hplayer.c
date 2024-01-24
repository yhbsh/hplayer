#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
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

    SDL_Window *window = SDL_CreateWindow("Hplayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    AVFormatContext *avformat_context = NULL;

    avformat_open_input(&avformat_context, argv[1], NULL, NULL);
    avformat_find_stream_info(avformat_context, NULL);

    const AVStream *av_stream = avformat_context->streams[0];
    const AVCodecParameters *avcodec_parameters = av_stream->codecpar;
    enum AVCodecID avcodec_id = avcodec_parameters->codec_id;

    const AVCodec *avcodec = avcodec_find_decoder(avcodec_id);
    AVCodecContext *avcodec_context = avcodec_alloc_context3(avcodec);

    avcodec_parameters_to_context(avcodec_context, avcodec_parameters);
    avcodec_open2(avcodec_context, avcodec, NULL);

    AVFrame *av_frame = av_frame_alloc();
    AVPacket av_packet;

    bool quit = false;
    while (!quit) {

        if (av_read_frame(avformat_context, &av_packet) < 0)
            quit = true;

        avcodec_send_packet(avcodec_context, &av_packet);
        while (avcodec_receive_frame(avcodec_context, av_frame) == 0) {
            SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, av_frame->width, av_frame->height);

						uint8_t *yplane  = av_frame->data[0];
						const int ylinesize = av_frame->linesize[0];
						uint8_t *uplane  = av_frame->data[1];
						const int ulinesize = av_frame->linesize[1];
						uint8_t *vplane  = av_frame->data[2];
						const int vlinesize = av_frame->linesize[2];

            SDL_UpdateYUVTexture(texture, NULL, yplane, ylinesize, uplane, ulinesize, vplane, vlinesize);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        av_packet_unref(&av_packet);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
        }

        SDL_Delay(1000 / 60);
    }

    av_frame_free(&av_frame);
    avcodec_free_context(&avcodec_context);
    avformat_close_input(&avformat_context);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
