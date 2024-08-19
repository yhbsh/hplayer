#ifndef PL_H
#define PL_H

#define PL_API

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>

typedef struct {
    /* FFmpeg */
    AVFormatContext *format_context;
    AVStream *video_stream;
    AVStream *audio_stream;
    AVCodecContext *video_codec_context;
    AVCodecContext *audio_codec_context;
    AVPacket *packet;
    AVFrame *frame;

    /* GLFW */
    GLFWwindow *window;

    /* OpenGL */
    GLuint prog, vao, vbo, textures[3];

} PlayerEngine;

PL_API int pl_engine_init(PlayerEngine **p_engine, const char *url);
PL_API void pl_engine_deinit(PlayerEngine **p_engine);

#endif // PL_H
