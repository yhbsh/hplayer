#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <pthread.h>
#include <stdio.h>

int64_t app_time;
int ret;

static GLFWwindow *window = NULL;

static AVFormatContext *format_context = NULL;
static AVCodecContext *codec_context   = NULL;
static const AVCodec *codec            = NULL;
static AVStream *stream                = NULL;
static AVPacket *packet                = NULL;
static AVFrame *frame                  = NULL;

static GLuint textureY, textureU, textureV, program;

static pthread_t thread;
static pthread_mutex_t mutex;
static pthread_cond_t cond;

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

void init_ffmpeg(const char *url);
void init_window(GLFWwindow **window);
void init_textures(GLuint *textureY, GLuint *textureU, GLuint *textureV, GLuint program);
void init_compile_link_gl_program(GLuint *program);

int main(int argc, const char *argv[]) {
    app_time = av_gettime_relative();

    if (argc != 2) {
        printf("USAGE: ./main <url>\n");
        return 1;
    }

    init_window(&window);
    init_compile_link_gl_program(&program);
    init_textures(&textureY, &textureU, &textureV, program);
    init_ffmpeg(argv[1]);

    while (!glfwWindowShouldClose(window)) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);

        // printf("VIDEO %05lld | PTS %llds %03lldms %03lldus | RTS %llds %03lldms %03lldus\n", vcodec_context->frame_num, pts_seconds, pts_milliseconds, pts_microseconds, rts_seconds, rts_milliseconds, rts_microseconds);
        printf("| %p | %p | %p | %d | %d |\n", frame->data[0], frame->data[1], frame->data[2], frame->width, frame->height);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
        glfwPollEvents();

        pthread_mutex_unlock(&mutex);
    }

    return 0;
}

void *run_ffmpeg(void *arg) {
    while (1) {
        ret = av_read_frame(format_context, packet);
        if (ret == AVERROR_EOF) break;
        if (ret == AVERROR(EAGAIN)) continue;

        if (packet->stream_index != stream->index) {
            av_packet_unref(packet);
            continue;
        }

        ret = avcodec_send_packet(codec_context, packet);
        while (ret >= 0) {
            ret = avcodec_receive_frame(codec_context, frame);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;
            if (ret < 0) exit(1);

            int64_t pts = (1000 * 1000 * frame->pts * stream->time_base.num) / stream->time_base.den;
            int64_t rts = av_gettime_relative() - app_time;

            int64_t pts_seconds      = (pts / 1000000);
            int64_t pts_milliseconds = (pts % 1000000) / 1000;
            int64_t pts_microseconds = pts % 1000;

            int64_t rts_seconds      = (rts / 1000000);
            int64_t rts_milliseconds = (rts % 1000000) / 1000;
            int64_t rts_microseconds = rts % 1000;

            if (pts > rts) {
                pthread_mutex_lock(&mutex);
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&mutex);
                av_usleep(pts - rts);
            }
        }

        av_packet_unref(packet);
    }

    return NULL;
}

void init_ffmpeg(const char *url) {
    if ((ret = avformat_open_input(&format_context, url, NULL, NULL)) < 0) exit(1);
    if ((ret = avformat_find_stream_info(format_context, NULL)) < 0) exit(1);
    if ((ret = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0) exit(1);

    stream = format_context->streams[ret];

    if ((codec_context = avcodec_alloc_context3(codec)) == NULL) exit(1);
    if ((ret = avcodec_parameters_to_context(codec_context, stream->codecpar)) < 0) exit(1);
    if ((ret = avcodec_open2(codec_context, codec, NULL)) < 0) exit(1);
    if ((packet = av_packet_alloc()) == NULL) exit(1);
    if ((frame = av_frame_alloc()) == NULL) exit(1);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_create(&thread, NULL, run_ffmpeg, &url);
}
void init_window(GLFWwindow **window) {
    if ((ret = glfwInit()) < 0) return;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    if ((*window = glfwCreateWindow(1280, 720, "VIDEO", NULL, NULL)) == NULL) return;

    glfwSwapInterval(0);
    glfwMakeContextCurrent(*window);
    glfwSwapInterval(0);
}

void init_compile_link_gl_program(GLuint *program) {
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
