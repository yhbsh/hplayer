#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <stdio.h>

static const char *vertex_shader_source = "#version 410\n"
                                          "layout (location = 0) in vec2 position;\n"
                                          "layout (location = 1) in vec2 texCoord;\n"
                                          "out vec2 texCoordVarying;\n"
                                          "void main() {\n"
                                          "    gl_Position = vec4(position, 0.0, 1.0);\n"
                                          "    texCoordVarying = texCoord;\n"
                                          "}\n";

static const char *fragment_shader_source_yuv420p = "#version 410 core\n"
                                                    "in vec2 texCoordVarying;\n"
                                                    "uniform sampler2D textureY;\n"
                                                    "uniform sampler2D textureU;\n"
                                                    "uniform sampler2D textureV;\n"
                                                    "out vec4 fragColor;\n"
                                                    "void main() {\n"
                                                    "    float r, g, b, y, u, v;\n"
                                                    "    y = texture(textureY, texCoordVarying).r;\n"
                                                    "    u = texture(textureU, texCoordVarying).r;\n"
                                                    "    v = texture(textureV, texCoordVarying).r;\n"
                                                    "    y = 1.1643 * (y-0.0625);\n"
                                                    "    u = u - 0.5;\n"
                                                    "    v = v - 0.5;\n"
                                                    "    r = y + 1.5958 * v;\n"
                                                    "    g = y - 0.39173 * u - 0.81290 * v;\n"
                                                    "    b = y + 2.017 * u;\n"
                                                    "    fragColor = vec4(r, g, b, 1.0);\n"
                                                    "}\n";

static const char *fragment_shader_source_yuvj420p = "#version 410\n"
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
                                                     "    rgb = mat3(1.0, 1.0, 1.0,\n"
                                                     "               0.0, -0.344, 1.772,\n"
                                                     "               1.402, -0.714, 0.0) * yuv;\n"
                                                     "    fragColor = vec4(rgb, 1.0);\n"
                                                     "}\n";

static const char *fragment_shader_source_yuv420p10le = "#version 410\n"
                                                        "in vec2 texCoordVarying;\n"
                                                        "uniform sampler2D textureY;\n"
                                                        "uniform sampler2D textureU;\n"
                                                        "uniform sampler2D textureV;\n"
                                                        "out vec4 fragColor;\n"
                                                        "void main() {\n"
                                                        "    vec3 yuv;\n"
                                                        "    vec3 rgb;\n"
                                                        "    yuv.x = texture(textureY, texCoordVarying).r * (1.0 / 1023.0);\n"
                                                        "    yuv.y = texture(textureU, texCoordVarying).r * (1.0 / 1023.0) - 0.5;\n"
                                                        "    yuv.z = texture(textureV, texCoordVarying).r * (1.0 / 1023.0) - 0.5;\n"
                                                        "    rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.344136, 1.772000, 1.402000, -0.714136, 0.0) * yuv;\n"
                                                        "    fragColor = vec4(rgb, 1.0);\n"
                                                        "}\n";

static const char *fragment_shader_source_nv12 = "#version 410\n"
                                                 "in vec2 texCoordVarying;\n"
                                                 "uniform sampler2D textureY;\n"
                                                 "uniform sampler2D textureUV;\n"
                                                 "out vec4 fragColor;\n"
                                                 "void main() {\n"
                                                 "    vec3 yuv;\n"
                                                 "    vec3 rgb;\n"
                                                 "    // Sample the Y component\n"
                                                 "    yuv.x = texture(textureY, texCoordVarying).r;\n"
                                                 "    // Sample the UV components (UV are interleaved)\n"
                                                 "    vec2 uv = texture(textureUV, texCoordVarying).rg;\n"
                                                 "    yuv.y = uv.x - 0.5;\n"
                                                 "    yuv.z = uv.y - 0.5;\n"
                                                 "    // Conversion from YUV to RGB\n"
                                                 "    rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.344, 1.772, 1.402, -0.714, 0.0) * yuv;\n"
                                                 "    fragColor = vec4(rgb, 1.0);\n"
                                                 "}\n";

void init_window(GLFWwindow **window) {
    int ret = 0;

    if ((ret = glfwInit()) < 0) {
        return;
    }

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_REFRESH_RATE, 180);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    if ((*window = glfwCreateWindow(1920, 1080, "VIDEO", NULL, NULL)) == NULL) {
        return;
    }

    glfwMakeContextCurrent(*window);
}

void init_compile_link_gl_program(GLuint *program, const char *file_path) {
    GLuint vertex_shader, fragment_shader;

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source_yuv420p, NULL);
    // glShaderSource(fragment_shader, 1, &fragment_shader_source_yuv420p10le, NULL);
    // glShaderSource(fragment_shader, 1, &fragment_shader_source_yuvj420p, NULL);
    // glShaderSource(fragment_shader, 1, &fragment_shader_source_nv12, NULL);
    glCompileShader(fragment_shader);

    *program = glCreateProgram();
    glAttachShader(*program, vertex_shader);
    glAttachShader(*program, fragment_shader);
    glLinkProgram(*program);

    glUseProgram(*program);

    glDetachShader(*program, vertex_shader);
    glDetachShader(*program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void init_textures(GLuint *textureY, GLuint *textureU, GLuint *textureV, GLuint program) {
    glGenTextures(1, textureY);
    glBindTexture(GL_TEXTURE_2D, *textureY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, textureU);
    glBindTexture(GL_TEXTURE_2D, *textureU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, textureV);
    glBindTexture(GL_TEXTURE_2D, *textureV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLfloat vertices[] = {
        // clang-format off
        -1.0f, +1.0f, +0.0f, +0.0f,
        -1.0f, -1.0f, +0.0f, +1.0f,
        +1.0f, +1.0f, +1.0f, +0.0f,
        +1.0f, -1.0f, +1.0f, +1.0f,
        // clang-format on
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint positionAttrib = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);

    GLuint texCoordAttrib = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(texCoordAttrib);
    glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));

    glUniform1i(glGetUniformLocation(program, "textureY"), 0);
    glUniform1i(glGetUniformLocation(program, "textureU"), 1);
    glUniform1i(glGetUniformLocation(program, "textureV"), 2);
}

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        printf("USAGE: ./main <url>\n");
        return 1;
    }

    int ret;
    GLFWwindow *window              = NULL;
    const AVCodec *vcodec           = NULL;
    const AVCodec *acodec           = NULL;
    AVFormatContext *format_context = NULL;
    AVCodecContext *vcodec_context  = NULL;
    AVCodecContext *acodec_context  = NULL;
    AVStream *vstream               = NULL;
    AVStream *astream               = NULL;
    AVPacket *packet                = NULL;
    AVFrame *frame                  = NULL;
    GLuint textureY, textureU, textureV, program;

    // av_log_set_level(AV_LOG_TRACE);

    init_window(&window);
    init_compile_link_gl_program(&program, "");
    init_textures(&textureY, &textureU, &textureV, program);

    if ((ret = avformat_open_input(&format_context, argv[1], NULL, NULL)) < 0) {
        fprintf(stderr, "[ERROR]: avformat_open_input: %s\n", av_err2str(ret));
        return ret;
    }
    if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) {
        fprintf(stderr, "[ERROR]: avformat_find_stream_info: %s\n", av_err2str(ret));
        return ret;
    }

    if ((ret = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &vcodec, 0)) >= 0) {
        vstream = format_context->streams[ret];
        if ((vcodec_context = avcodec_alloc_context3(vcodec)) == NULL) {
            printf("No Video\n");
            return ret;
        }
        if ((ret = avcodec_parameters_to_context(vcodec_context, vstream->codecpar)) < 0) {
            fprintf(stderr, "[ERROR]: avcodec_parameters_to_context: %s\n", av_err2str(ret));
            return ret;
        }
        if ((ret = avcodec_open2(vcodec_context, vcodec, NULL)) < 0) {
            return ret;
        }
    } else {
        fprintf(stderr, "[WARNING]: %s\n", av_err2str(ret));
    }

    if ((ret = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &acodec, 0)) >= 0) {
        astream = format_context->streams[ret];
        if ((acodec_context = avcodec_alloc_context3(acodec)) == NULL) {
            printf("No Audio\n");
        }
        if ((ret = avcodec_parameters_to_context(acodec_context, astream->codecpar)) < 0) {
            fprintf(stderr, "[ERROR]: avcodec_parameters_to_context: %s\n", av_err2str(ret));
            return ret;
        }
        if ((ret = avcodec_open2(acodec_context, acodec, NULL)) < 0) {
            return ret;
        }
    } else {
        fprintf(stderr, "[WARNING]: %s\n", av_err2str(ret));
    }

    if ((packet = av_packet_alloc()) == NULL) {
        return 1;
    }

    if ((frame = av_frame_alloc()) == NULL) {
        return 1;
    }

    int64_t begin = av_gettime_relative();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ret = av_read_frame(format_context, packet);
        if (ret == AVERROR_EOF) break;
        if (ret == AVERROR(EAGAIN)) continue;

        if (astream && acodec_context && packet->stream_index == astream->index) {
            ret = avcodec_send_packet(acodec_context, packet);
            while (ret >= 0) {
                ret = avcodec_receive_frame(acodec_context, frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;
                if (ret < 0) {
                    fprintf(stderr, "avcodec_receive_frame: %s\n", av_err2str(ret));
                    return 1;
                }

                int64_t pts = (1000 * 1000 * frame->pts * astream->time_base.num) / astream->time_base.den;
                int64_t rts = av_gettime_relative() - begin;

                int64_t pts_seconds      = (pts / 1000000);
                int64_t pts_milliseconds = (pts % 1000000) / 1000;
                int64_t pts_microseconds = pts % 1000;

                int64_t rts_seconds      = (rts / 1000000);
                int64_t rts_milliseconds = (rts % 1000000) / 1000;
                int64_t rts_microseconds = rts % 1000;

                printf("AUDIO %05lld | PTS %llds %03lldms %03lldus | RTS %llds %03lldms %03lldus\n", acodec_context->frame_num, pts_seconds, pts_milliseconds, pts_microseconds, rts_seconds, rts_milliseconds, rts_microseconds);

                // if (pts > rts) {
                //     if ((ret = av_usleep(pts - rts)) < 0) {
                //         fprintf(stderr, "[ERROR]: av_usleep: %s\n", av_err2str(ret));
                //     }
                //}
            }
        }

        if (vstream && vcodec_context && packet->stream_index == vstream->index) {
            ret = avcodec_send_packet(vcodec_context, packet);
            while (ret >= 0) {
                ret = avcodec_receive_frame(vcodec_context, frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;
                if (ret < 0) {
                    fprintf(stderr, "avcodec_receive_frame: %s\n", av_err2str(ret));
                    return 1;
                }

                int64_t pts = (1000 * 1000 * frame->pts * vstream->time_base.num) / vstream->time_base.den;
                int64_t rts = av_gettime_relative() - begin;

                int64_t pts_seconds      = (pts / 1000000);
                int64_t pts_milliseconds = (pts % 1000000) / 1000;
                int64_t pts_microseconds = pts % 1000;

                int64_t rts_seconds      = (rts / 1000000);
                int64_t rts_milliseconds = (rts % 1000000) / 1000;
                int64_t rts_microseconds = rts % 1000;

                printf("VIDEO %05lld | PTS %llds %03lldms %03lldus | RTS %llds %03lldms %03lldus\n", acodec_context->frame_num, pts_seconds, pts_milliseconds, pts_microseconds, rts_seconds, rts_milliseconds, rts_microseconds);

                if (pts > rts) {
                    if ((ret = av_usleep(pts - rts)) < 0) {
                        fprintf(stderr, "[ERROR]: av_usleep: %s\n", av_err2str(ret));
                    }
                }

                glClear(GL_COLOR_BUFFER_BIT);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureY);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 1, frame->height / 1, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, textureU);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, textureV);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glfwSwapBuffers(window);
            }
        }

        av_packet_unref(packet);
    }

    return 0;
}
