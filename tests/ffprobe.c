#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./ffprobe [file]\n");
        return 1;
    }

    int ret;
    AVFormatContext *format_context = NULL;
    if ((ret = avformat_open_input(&format_context, argv[1], NULL, NULL)) < 0) {
        return 1;
    }

    if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) {
        return 1;
    }

    av_dump_format(format_context, 0, argv[1], 0);

    return 0;
}
