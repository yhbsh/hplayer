#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool load_frame(const char *fp, int *w, int *h, unsigned char **buff) {
  int ret = 0;

  AVFormatContext *avformat_ctx = avformat_alloc_context();
  ret = avformat_open_input(&avformat_ctx, fp, NULL, NULL);
  if (ret != 0) {
    fprintf(stderr, "[ERROR]: avformat_open_input: %s\n", av_err2str(ret));
    return false;
  }

  ret = av_find_best_stream(avformat_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (ret < 0) {
    fprintf(stderr, "[ERROR]: av_find_best_stream: %s\n", av_err2str(ret));
    return false;
  }

  AVStream *vs = avformat_ctx->streams[ret];
  AVCodecParameters *avcodec_params = vs->codecpar;
  const AVCodec *avcodec = avcodec_find_decoder(avcodec_params->codec_id);
  if (avcodec == NULL) {
    fprintf(stderr, "[ERROR]: avcodec_find_decoder: could not find decoder for %s codec\n", av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
    return false;
  }

  AVCodecContext *avcodec_ctx = avcodec_alloc_context3(avcodec);
  if (avcodec_ctx == NULL) {
    fprintf(stderr, "[ERROR]: avcodec_alloc_context3 failed\n");
    return false;
  }

  ret = avcodec_parameters_to_context(avcodec_ctx, avcodec_params);
  if (ret < 0) {
    fprintf(stderr, "[ERROR]: avcodec_parameters_to_context: %s\n", av_err2str(ret));
    return false;
  }

  ret = avcodec_open2(avcodec_ctx, avcodec, NULL);
  if (ret < 0) {
    fprintf(stderr, "[ERROR]: avcodec_open2: %s\n", av_err2str(ret));
    return false;
  }

  AVFrame *avframe = av_frame_alloc();
  if (avframe == NULL) {
    fprintf(stderr, "[ERROR]: av_frame_alloc\n");
    return false;
  }

  AVPacket *avpacket = av_packet_alloc();
  if (avpacket == NULL) {
    fprintf(stderr, "[ERROR]: av_packet_alloc\n");
    return false;
  }

  while (av_read_frame(avformat_ctx, avpacket) >= 0) {
    if (avpacket->stream_index != vs->index) {
      continue;
    }

    ret = avcodec_send_packet(avcodec_ctx, avpacket);
    if (ret < 0) {
      fprintf(stderr, "[ERROR]: avcodec_send_packet: %s\n", av_err2str(ret));
      return false;
    }

    ret = avcodec_receive_frame(avcodec_ctx, avframe);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      continue;
    }
    if (ret < 0) {
      fprintf(stderr, "[ERROR]: avcodec_receive_frame: %s\n", av_err2str(ret));
      return false;
    }
    av_packet_unref(avpacket);
    break;
  }

  struct SwsContext *sws_scaler_ctx = sws_getContext(avframe->width, avframe->height, avcodec_ctx->pix_fmt, avframe->width, avframe->height, AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL, NULL, NULL);

  if (sws_scaler_ctx == NULL) {
    fprintf(stderr, "[ERROR]: sws_getContext\n");
    return false;
  }

  unsigned char *data = malloc(avframe->width * avframe->height * 4);
  unsigned char *dest[4] = {data, NULL, NULL, NULL};
  int dest_linesize[4] = {avframe->width * 4, 0, 0, 0};
  sws_scale(sws_scaler_ctx, (const unsigned char *const *)avframe->data, avframe->linesize, 0, avframe->height, dest, dest_linesize);
  sws_freeContext(sws_scaler_ctx);

  *w = avframe->width;
  *h = avframe->height;
  *buff = data;

  avformat_close_input(&avformat_ctx);
  avformat_free_context(avformat_ctx);
  av_frame_free(&avframe);
  av_packet_free(&avpacket);
  avcodec_free_context(&avcodec_ctx);

  return true;
}

int main(void) {
  const char *fp = "samples/s2.mp4";
  int x, y;

  unsigned char *buff;
  if (!load_frame(fp, &x, &y, &buff)) {
    fprintf(stderr, "[ERROR]: could not load video frame\n");
    return 1;
  }

  glfwInit();
  GLFWwindow *w = glfwCreateWindow(x, y, "Window", NULL, NULL);
  glfwMakeContextCurrent(w);
  printf("version = %s\n", glGetString(GL_VERSION));

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, buff);

  while (!glfwWindowShouldClose(w)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, x, y, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_QUADS);
    glTexCoord2d(0, 0);
    glVertex2i(0, 0);
    glTexCoord2d(1, 0);
    glVertex2i(x, 0);
    glTexCoord2d(1, 1);
    glVertex2i(x, y);
    glTexCoord2d(0, 1);
    glVertex2i(0, y);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glfwSwapBuffers(w);
    glfwWaitEvents();
  }

  glfwTerminate();

  return 0;
}
