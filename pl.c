#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>

#include "pl.h"

struct PL_Engine {
    /* FFmpeg */
    AVFormatContext *format_context;
    AVStream *video_stream;
    AVStream *audio_stream;
    AVCodecContext *video_codec_context;
    AVCodecContext *audio_codec_context;
    AVPacket *packet;
    AVFrame *frame;

    int64_t start_time;
    int64_t pts_time;

    /* GLFW */
    GLFWwindow *window;

    /* OpenGL */
    GLuint prog, vao, vbo, textures[3];
};

int init_ffmpeg(PL_Engine *pl_engine, const char *url) {
    int ret;
    av_log_set_level(AV_LOG_DEBUG);

    if ((ret = avformat_open_input(&pl_engine->format_context, url, NULL, NULL)) < 0) return ret;
    if ((ret = avformat_find_stream_info(pl_engine->format_context, NULL)) < 0) return ret;

    const AVCodec *video_codec = NULL;
    const AVCodec *audio_codec = NULL;

    ret = av_find_best_stream(pl_engine->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    if (ret >= 0) {
        if (!(pl_engine->video_stream = pl_engine->format_context->streams[ret])) return AVERROR(ENOMEM);
        if (!(pl_engine->video_codec_context = avcodec_alloc_context3(video_codec))) return AVERROR(ENOMEM);
        if ((ret = avcodec_parameters_to_context(pl_engine->video_codec_context, pl_engine->video_stream->codecpar)) < 0) return ret;
        if ((ret = avcodec_open2(pl_engine->video_codec_context, video_codec, NULL)) < 0) return ret;
    }

    ret = av_find_best_stream(pl_engine->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &audio_codec, 0);
    if (ret >= 0) {
        if (!(pl_engine->audio_stream = pl_engine->format_context->streams[ret])) return AVERROR(ENOMEM);
        if (!(pl_engine->audio_codec_context = avcodec_alloc_context3(video_codec))) return AVERROR(ENOMEM);
        if ((ret = avcodec_parameters_to_context(pl_engine->audio_codec_context, pl_engine->audio_stream->codecpar)) < 0) return ret;
        if ((ret = avcodec_open2(pl_engine->audio_codec_context, video_codec, NULL)) < 0) return ret;
    }

    if (!(pl_engine->packet = av_packet_alloc())) return AVERROR(ENOMEM);
    if (!(pl_engine->frame = av_frame_alloc())) return AVERROR(ENOMEM);

    return 0;
}

void deinit_ffmpeg(PL_Engine *pl_engine) {
    av_packet_free(&pl_engine->packet);
    av_frame_free(&pl_engine->frame);
    avcodec_free_context(&pl_engine->video_codec_context);
    avcodec_free_context(&pl_engine->audio_codec_context);
    avformat_close_input(&pl_engine->format_context);
}

void error_callback(int error, const char *description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)action;
    (void)mods;

    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    float aspect_ratio = 16.0f / 9.0f;
    int new_width      = width;
    int new_height     = height;

    if (width / aspect_ratio > height) {
        new_width = height * aspect_ratio;
    } else {
        new_height = width / aspect_ratio;
    }

    int viewport_x = (width - new_width) / 2;
    int viewport_y = (height - new_height) / 2;

    glViewport(viewport_x, viewport_y, new_width, new_height);
}

int init_glfw(PL_Engine *pl_engine) {
    if (!glfwInit()) return -1;

    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor *monitor    = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    int factor = 100;
    int width  = 16 * factor;
    int height = 9 * factor;

    if (!(pl_engine->window = glfwCreateWindow(width, height, "PL", NULL, NULL))) return -1;

    glfwSetKeyCallback(pl_engine->window, key_callback);
    glfwSetFramebufferSizeCallback(pl_engine->window, framebuffer_size_callback);

    glfwMakeContextCurrent(pl_engine->window);
    glfwSwapInterval(0);

    glfwSetWindowPos(pl_engine->window, (mode->width - width) / 2, (mode->height - height) / 2);

    return 0;
}

void deinit_glfw(PL_Engine *pl_engine) {
    if (pl_engine->window) glfwDestroyWindow(pl_engine->window);
    glfwTerminate();
}

const char *vert_src = "#version 410\n"
                       "layout (location = 0) in vec2 position;\n"
                       "layout (location = 1) in vec2 texCoord;\n"
                       "out vec2 texCoordVarying;\n"
                       "void main() {\n"
                       "    gl_Position = vec4(position, 0.0, 1.0);\n"
                       "    texCoordVarying = texCoord;\n"
                       "}\n";

const char *frag_src = "#version 410\n"
                       "in vec2 texCoordVarying;\n"
                       "uniform sampler2D textureY;\n"
                       "uniform sampler2D textureU;\n"
                       "uniform sampler2D textureV;\n"
                       "out vec4 fragColor;\n"
                       "void main() {\n"
                       "    vec3 yuv;\n"
                       "    vec3 rgb;\n"
                       "    yuv.x = texture(textureY, texCoordVarying).r;\n"
                       "    yuv.y = texture(textureU, texCoordVarying).r - 0.5;\n"
                       "    yuv.z = texture(textureV, texCoordVarying).r - 0.5;\n"
                       "    rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.344, 1.772, 1.402, -0.714, 0.0) * yuv;\n"
                       "    fragColor = vec4(rgb, 1.0);\n"
                       "}\n";

int init_opengl(PL_Engine *pl_engine) {
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);

    GLint status;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vert, 512, NULL, infoLog);
        printf("Vertex Shader Compilation Failed: %s\n", infoLog);
        return -1;
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);

    glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar infoLog[512];
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        printf("Fragment Shader Compilation Failed: %s\n", infoLog);
        return -1;
    }

    pl_engine->prog = glCreateProgram();
    glAttachShader(pl_engine->prog, vert);
    glAttachShader(pl_engine->prog, frag);
    glLinkProgram(pl_engine->prog);

    glGetProgramiv(pl_engine->prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(pl_engine->prog, 512, NULL, infoLog);
        printf("Shader Program Linking Failed: %s\n", infoLog);
        return -1;
    }

    glUseProgram(pl_engine->prog);

    glGenTextures(3, pl_engine->textures);

    // clang-format off
    GLfloat vertices[] = {
        -1.0f, +1.0f, +0.0f, +0.0f,
        -1.0f, -1.0f, +0.0f, +1.0f,
        +1.0f, +1.0f, +1.0f, +0.0f,
        +1.0f, -1.0f, +1.0f, +1.0f,
    };
    // clang-format on

    glGenVertexArrays(1, &pl_engine->vao);
    glBindVertexArray(pl_engine->vao);

    glGenBuffers(1, &pl_engine->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, pl_engine->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint positionAttrib = glGetAttribLocation(pl_engine->prog, "position");
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);

    GLuint texCoordAttrib = glGetAttribLocation(pl_engine->prog, "texCoord");
    glEnableVertexAttribArray(texCoordAttrib);
    glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));

    glUniform1i(glGetUniformLocation(pl_engine->prog, "textureY"), 0);
    glUniform1i(glGetUniformLocation(pl_engine->prog, "textureU"), 1);
    glUniform1i(glGetUniformLocation(pl_engine->prog, "textureV"), 2);

    // Clean up shaders
    glDetachShader(pl_engine->prog, vert);
    glDetachShader(pl_engine->prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    return 0;
}

void deinit_opengl(PL_Engine *pl_engine) {
    glDeleteProgram(pl_engine->prog);
    glDeleteVertexArrays(1, &pl_engine->vao);
    glDeleteBuffers(1, &pl_engine->vbo);
    glDeleteTextures(3, pl_engine->textures);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

int pl_engine_init(PL_Engine **o_pl_engine, const char *url) {
    int ret;

    PL_Engine *pl_engine = (PL_Engine *)av_calloc(1, sizeof(PL_Engine));

    if ((ret = init_ffmpeg(pl_engine, url)) < 0) return ret;
    if ((ret = init_glfw(pl_engine)) < 0) return ret;
    if ((ret = init_opengl(pl_engine)) < 0) return ret;

    pl_engine->start_time = av_gettime_relative();
    pl_engine->pts_time   = 0;

    *o_pl_engine = pl_engine;

    return 0;
}

void pl_engine_deinit(PL_Engine **pl_engine) {
    deinit_ffmpeg(*pl_engine);
    deinit_glfw(*pl_engine);
    deinit_opengl(*pl_engine);

    free(*pl_engine);
    *pl_engine = NULL;
}

int pl_engine_is_vpacket(PL_Engine *pl_engine) {
    return pl_engine->packet->stream_index == pl_engine->video_stream->index;
}

int pl_engine_is_apacket(PL_Engine *pl_engine) {
    return pl_engine->packet->stream_index == pl_engine->audio_stream->index;
}

int pl_window_should_close(PL_Engine *pl_engine) {
    return glfwWindowShouldClose(pl_engine->window);
}

int pl_read_packet(PL_Engine *pl_engine) {
    int ret = av_read_frame(pl_engine->format_context, pl_engine->packet);
    if (ret == AVERROR(EAGAIN)) return PL_ERROR_WOULD_BLOCK;
    if (ret == AVERROR_EOF) return PL_ERROR_EOF;

    return ret;
}

int pl_send_vpacket(PL_Engine *pl_engine) {
    return avcodec_send_packet(pl_engine->video_codec_context, pl_engine->packet);
}

int pl_send_apacket(PL_Engine *pl_engine) {
    return avcodec_send_packet(pl_engine->audio_codec_context, pl_engine->packet);
}

void pl_packet_unref(PL_Engine *pl_engine) {
    return av_packet_unref(pl_engine->packet);
}

int pl_receive_vframe(PL_Engine *pl_engine) {
    int ret = avcodec_receive_frame(pl_engine->video_codec_context, pl_engine->frame);
    if (ret == AVERROR_EOF) return PL_ERROR_EOF;
    if (ret == AVERROR(EAGAIN)) return PL_ERROR_WOULD_BLOCK;
    return ret;
}

int pl_receive_aframe(PL_Engine *pl_engine) {
    return avcodec_receive_frame(pl_engine->audio_codec_context, pl_engine->frame);
}

void pl_render_frame(PL_Engine *pl_engine) {
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pl_engine->textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pl_engine->frame->width, pl_engine->frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, pl_engine->frame->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pl_engine->textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pl_engine->frame->width / 2, pl_engine->frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, pl_engine->frame->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, pl_engine->textures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pl_engine->frame->width / 2, pl_engine->frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, pl_engine->frame->data[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glfwPollEvents();
    glfwSwapBuffers(pl_engine->window);
}

void pl_delay_pts(PL_Engine *pl_engine) {
    int64_t pts = pl_engine->frame->best_effort_timestamp;
    if (pts == AV_NOPTS_VALUE) pts = 0;
    pl_engine->pts_time  = av_rescale_q(pts, pl_engine->video_stream->time_base, AV_TIME_BASE_Q);
    int64_t current_time = av_gettime_relative() - pl_engine->start_time;
    int64_t delay        = (pl_engine->pts_time - current_time);

    if (delay > 0) av_usleep(delay);
}

PL_API const char *pl_err2str(int code) {
    return av_err2str(code);
}
