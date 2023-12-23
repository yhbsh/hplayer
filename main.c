#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>

int main(void) {

    AVFormatContext *avformat_context = NULL;

    avformat_open_input(&avformat_context, "file.mp4", NULL, NULL);
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
    while (1) {

        if (av_read_frame(avformat_context, &av_packet) < 0)
            break;

        avcodec_send_packet(avcodec_context, &av_packet);
        while (avcodec_receive_frame(avcodec_context, av_frame) == 0) {
            // printf("%d\n", *av_frame->linesize);
        }

        av_packet_unref(&av_packet);
    }

    av_frame_free(&av_frame);
    avcodec_free_context(&avcodec_context);
    avformat_close_input(&avformat_context);

    return 0;
}
