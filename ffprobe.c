#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "[USAGE]: ./ffprobe [file]\n");
    return 1;
  }

  AVFormatContext *format_context = NULL;
  int ret;
  const char *fp = argv[1];

  ret = avformat_open_input(&format_context, fp, NULL, NULL);
  if (ret < 0) {
    fprintf(stderr, "[ERROR]: avformat_open_input: %s\n", av_err2str(ret));
    return 1;
  }

  ret = avformat_find_stream_info(format_context, NULL);
  if (ret < 0) {
    fprintf(stderr, "[ERROR]: avformat_find_stream_info: %s\n", av_err2str(ret));
    return 1;
  }

  const AVStream *stream = format_context->streams[1];
  enum AVCodecID codec_id = stream->codecpar->codec_id;
  const AVCodec *codec = avcodec_find_decoder(codec_id);
  if (codec == NULL) {
    perror("avcodec_find_decoder()");
    return 1;
  }

  av_dump_format(format_context, 0, fp, 0);
  printf("name = %s\nlong_name = %s\n", codec->name, codec->long_name);

  return 0;
}
