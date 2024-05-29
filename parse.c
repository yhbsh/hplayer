#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./main <file.h264>\n");
        return 1;
    }

    AVPacket             *p   = av_packet_alloc();
    AVFrame              *f   = av_frame_alloc();
    const AVCodec        *c   = avcodec_find_decoder(AV_CODEC_ID_H264);
    AVCodecParserContext *cpc = av_parser_init(c->id);
    AVCodecContext       *cc  = avcodec_alloc_context3(c);
    avcodec_open2(cc, c, NULL);

    if (!p || !c || !cpc || !cc) return 1;

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
        int parse_size = av_parser_parse2(cpc, cc, &p->data, &p->size, data, remaining_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        data += parse_size;
        remaining_size -= parse_size;

        if (p->size) {

            int ret = avcodec_send_packet(cc, p);
            while (ret >= 0) {
                ret = avcodec_receive_frame(cc, f);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

                printf("Frame %6lld: width = %d, height = %d, format = %s\n", cc->frame_num, f->width, f->height, av_get_pix_fmt_name(f->format));
            }

            av_packet_unref(p);
        }
    }

clean:
    free(buffer);
    avcodec_free_context(&cc);
    av_parser_close(cpc);
    av_packet_free(&p);

    return 0;
}
