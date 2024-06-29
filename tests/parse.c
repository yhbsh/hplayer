#include <libavcodec/avcodec.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./main <file.hevc>\n");
        return 1;
    }

    av_log_set_level(AV_LOG_TRACE);
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    int ret = avcodec_open2(codec_context, codec, NULL);
    AVCodecParserContext *parser_context = av_parser_init(codec->id);

    FILE *file = fopen(argv[1], "rb");
    fseek(file, 0, SEEK_END);
    uint64_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *file_buffer = av_malloc(file_size + AV_INPUT_BUFFER_PADDING_SIZE);
    fread(file_buffer, 1, file_size, file);
    fclose(file);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    uint8_t *data = file_buffer;
    int data_size = file_size;

    while (data_size > 0) {
        ret        = av_parser_parse2(parser_context, codec_context, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        data      += ret;
        data_size -= ret;

        if (packet->size) {
            printf("packet->size: %10d\n", packet->size);
            while (avcodec_send_packet(codec_context, packet) >= 0) {
                ret = avcodec_receive_frame(codec_context, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                printf("%-5lld frame->width %5d - frame->height %5d\n", codec_context->frame_num, frame->width, frame->height);
            }
        }
    }

    av_packet_free(&packet);
    av_free(file_buffer);
    av_parser_close(parser_context);
    avcodec_free_context(&codec_context);

    return 0;
}

