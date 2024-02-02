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

    // Fill the format context with input details
    avformat_open_input(&avformat_context, argv[1], NULL, NULL);
    // Find stream info using the input details filled before
    avformat_find_stream_info(avformat_context, NULL);

    // Video
    const AVStream *av_stream = avformat_context->streams[0];
    // Video decoder Parameters (Codec stands for encoder/decoder)
    const AVCodecParameters *avcodec_parameters = av_stream->codecpar;
    // Codec id for the specific video stream
    enum AVCodecID avcodec_id = avcodec_parameters->codec_id;

    // Using the decoder id, find the right decoder for the specific video stream
    const AVCodec *avcodec = avcodec_find_decoder(avcodec_id);
    // Create a decoder context
    AVCodecContext *avcodec_context = avcodec_alloc_context3(avcodec);

    // Fill the decoder context using the decoder parameters found in the video stream
    avcodec_parameters_to_context(avcodec_context, avcodec_parameters);

    // Opening the decoder to start decoding
    avcodec_open2(avcodec_context, avcodec, NULL);

    // Create a frame (AVFrame is just an abstraction around a bitmap)
    AVFrame *av_frame = av_frame_alloc();
    // Create a packet (AVPacket represent one (or multiple frames) depending on the encoding algorithm used, this abstraction helps because we read packets and not individual frames, and then using
    // this packet we get the actual frames)
    AVPacket av_packet;

    bool quit = false;
    while (!quit) {

        // read the packet
        if (av_read_frame(avformat_context, &av_packet) < 0)
            quit = true;

        // send the packet to the decoder to decode it
        avcodec_send_packet(avcodec_context, &av_packet);

        // receive frames from the decoder after decoding the packet
        while (avcodec_receive_frame(avcodec_context, av_frame) == 0) {

            // draw the frame on a YUV plane using sdl
            SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, av_frame->width, av_frame->height);
            SDL_UpdateYUVTexture(texture, NULL, av_frame->data[0], av_frame->linesize[0], av_frame->data[1], av_frame->linesize[1], av_frame->data[2], av_frame->linesize[2]);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        // invalidate the packet since we do not need it now (we got the frames and we drew them)
        av_packet_unref(&av_packet);

        // handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
        }

        // make sure it plays at 60 frames per second
        SDL_Delay(16.667);
    }

    //    No need to deallocate anything, the OS will do it for us
    //    av_frame_free(&av_frame);
    //    avcodec_free_context(&avcodec_context);
    //    avformat_close_input(&avformat_context);
    //
    //    SDL_DestroyRenderer(renderer);
    //    SDL_DestroyWindow(window);

    return 0;
}
