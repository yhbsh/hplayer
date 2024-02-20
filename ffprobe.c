#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  int ret = 0;
  const char *fp = argv[1];
  AVFormatContext *avformat_ctx = NULL;

  if (argc != 2) {
    fprintf(stderr, "[USAGE]: ./ffprobe [file]\n");
    return 1;
  }

  ret = avformat_open_input(&avformat_ctx, fp, NULL, NULL);
  if (ret < 0) {
    fprintf(stderr, "[ERROR]: avformat_open_input: %s\n", av_err2str(ret));
    return 1;
  }

  ret = avformat_find_stream_info(avformat_ctx, NULL);
  if (ret < 0) {
    fprintf(stderr, "[ERROR]: avformat_find_stream_info: %s\n", av_err2str(ret));
    return 1;
  }

  av_dump_format(avformat_ctx, 0, fp, 0);

  return 0;
}
