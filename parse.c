#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./main <file.h264 | file.hevc>\n");
        return 1;
    }

    av_log_set_level(AV_LOG_TRACE);
    AVPacket             *in_packet   = av_packet_alloc();
    AVFrame              *in_frame   = av_frame_alloc();
    const AVCodec        *in_codec   = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!in_codec) {
        fprintf(stderr, "[ERROR]: codec does not exist\n");
        return 1;
    }
    AVCodecParserContext *in_codec_parser_ctx = av_parser_init(in_codec->id);
    AVCodecContext       *in_codec_ctx  = avcodec_alloc_context3(in_codec);
    avcodec_open2(in_codec_ctx, in_codec, NULL);
    int ret = -1;

    FILE *file = fopen(argv[1], "rb");

    fseek(file, 0, SEEK_END);
    uint64_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("File size: %lld\n", file_size);

    uint8_t *buffer = malloc(file_size);

    fread(buffer, 1, file_size, file);
    fclose(file);

    uint8_t *data           = buffer;
    uint64_t remaining_size = file_size;

    while (remaining_size > 0) {
        int parse_size = av_parser_parse2(in_codec_parser_ctx, in_codec_ctx, &in_packet->data, &in_packet->size, data, remaining_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        data += parse_size;
        remaining_size -= parse_size;

        if (in_packet->size) {

            ret = avcodec_send_packet(in_codec_ctx, in_packet);
            while (ret >= 0) {
                ret = avcodec_receive_frame(in_codec_ctx, in_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

                printf("Frame %6lld: width = %d, height = %d, format = %s\n", in_codec_ctx->frame_num, in_frame->width, in_frame->height, av_get_pix_fmt_name(in_frame->format));
            }

            av_packet_unref(in_packet);
        }
    }

clean:
    free(buffer);
    avcodec_free_context(&in_codec_ctx);
    av_parser_close(in_codec_parser_ctx);
    av_packet_free(&in_packet);

    return 0;
}
