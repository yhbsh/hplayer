#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

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

int init_engine(PlayerEngine **p_engine, const char *url);
void deinit_engine(PlayerEngine **p_engine);
