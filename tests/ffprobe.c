#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./ffprobe [file]\n");
        return 1;
    }

    AVFormatContext *in_ctx = NULL;
    int              ret;
    const char      *url = argv[1];
    ret                  = avformat_open_input(&in_ctx, url, NULL, NULL);
    ret                  = avformat_find_stream_info(in_ctx, NULL);
    const AVStream *s    = in_ctx->streams[1];
    const AVCodec  *c    = avcodec_find_decoder(s->codecpar->codec_id);

    av_dump_format(in_ctx, 0, url, 0);
    printf("name = %s\nlong_name = %s\n", c->name, c->long_name);

    return 0;
}
