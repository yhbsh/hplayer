#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/time.h>

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <assert.h>
#include <stdio.h>

int ret;
int64_t launch_time = 0;

static GLFWwindow *window = NULL;

static AVFormatContext *format_context = NULL;
static AVCodecContext *codec_context   = NULL;
static const AVCodec *codec            = NULL;
static AVStream *stream                = NULL;
static AVPacket *packet                = NULL;
static AVFrame *frame                  = NULL;

static GLuint textures[3], program, vertex_shader, fragment_shader;

static const char *vertex_shader_source = "#version 410\n"
                                          "layout (location = 0) in vec2 position;\n"
                                          "layout (location = 1) in vec2 texCoord;\n"
                                          "out vec2 texCoordVarying;\n"
                                          "void main() {\n"
                                          "    gl_Position = vec4(position, 0.0, 1.0);\n"
                                          "    texCoordVarying = texCoord;\n"
                                          "}\n";

static const char *fragment_shader_source = "#version 410 core\n"
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

int main(int argc, const char *argv[]) {
    launch_time = av_gettime_relative();

    if (argc != 2) {
        printf("USAGE: ./main <url>\n");
        return 1;
    }

    if ((ret = glfwInit()) < 0) return ret;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWmonitor *monitor    = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if ((window = glfwCreateWindow(mode->width, mode->height, "VIDEO", monitor, NULL)) == NULL) return 1;

    glfwSwapInterval(0);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glUseProgram(program);

    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glGenTextures(3, textures);

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

    // av_log_set_level(AV_LOG_TRACE);

    if ((ret = avformat_open_input(&format_context, argv[1], NULL, NULL)) < 0) exit(1);
    if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) exit(1);
    if ((ret = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0) exit(1);

    stream = format_context->streams[ret];

    if ((codec_context = avcodec_alloc_context3(codec)) == NULL) exit(1);
    if ((ret = avcodec_parameters_to_context(codec_context, stream->codecpar)) < 0) exit(1);
    if ((ret = avcodec_open2(codec_context, codec, NULL)) < 0) exit(1);
    if ((packet = av_packet_alloc()) == NULL) exit(1);
    if ((frame = av_frame_alloc()) == NULL) exit(1);

    while (!glfwWindowShouldClose(window)) {
        ret = av_read_frame(format_context, packet);
        if (ret == AVERROR_EOF) {
            av_seek_frame(format_context, -1, 0, AVSEEK_FLAG_ANY);
            continue;
        }

        if (ret == AVERROR(EAGAIN) || packet->stream_index != stream->index) {
            glfwPollEvents();
            av_packet_unref(packet);
            continue;
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            exit(0);
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            av_seek_frame(format_context, stream->index, 0, AVSEEK_FLAG_ANY);
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            av_seek_frame(format_context, stream->index, 0, AVSEEK_FLAG_ANY);
        }

        ret = avcodec_send_packet(codec_context, packet);
        while (ret >= 0) {
            ret = avcodec_receive_frame(codec_context, frame);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;
            if (ret < 0) exit(1);

            assert(frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUVJ420P);

            int64_t pts = (1000 * 1000 * frame->pts * stream->time_base.num) / stream->time_base.den;
            int64_t rts = av_gettime_relative() - launch_time;
            if (pts > rts) av_usleep(pts - rts);

            printf("%04lld | PTS %llds %03lldms %03lldus | RTS %llds %03lldms %03lldus | FMT: %s\n", codec_context->frame_num, pts / 1000000, (pts % 1000000) / 1000, pts % 1000, rts / 1000000, (rts % 1000000) / 1000, rts % 1000, av_get_pix_fmt_name(frame->format));

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 1, frame->height / 1, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textures[1]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, textures[2]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        av_packet_unref(packet);
    }

    return 0;
}
