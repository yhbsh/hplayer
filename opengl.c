#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>

#include <GLFW/glfw3.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./main [url]\n");
        return 1;
    }

    int              ret;
    AVFormatContext *in_ctx = NULL;
    ret                     = avformat_open_input(&in_ctx, argv[1], NULL, NULL);
    ret                     = avformat_find_stream_info(in_ctx, NULL);
    const AVCodec  *c       = NULL;
    int             vs      = av_find_best_stream(in_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &c, 0);
    const AVStream *s       = in_ctx->streams[vs];
    AVCodecContext *cc      = avcodec_alloc_context3(c);
    ret                     = avcodec_parameters_to_context(cc, s->codecpar);
    ret                     = avcodec_open2(cc, c, NULL);
    AVFrame  *f             = av_frame_alloc();
    AVPacket *p             = av_packet_alloc();

    const int width  = 960;
    const int height = 540;

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow *window = glfwCreateWindow(width, height, "Video", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *version     = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("Renderer: %s\nVendor: %s\nVersion: %s\nGLSL: %s\n", renderer, vendor, version, glslVersion);

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_src, NULL);
    glCompileShader(vert);
    GLint status;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &status);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glUseProgram(prog);

    GLuint textureY, textureU, textureV;
    glGenTextures(1, &textureY);
    glGenTextures(1, &textureV);
    glGenTextures(1, &textureU);

    // clang-format off
    GLfloat vertices[] = {
        -1.0, +1.0, +0.0, +0.0,
        -1.0, -1.0, +0.0, +1.0,
        +1.0, +1.0, +1.0, +0.0,
        +1.0, -1.0, +1.0, +1.0,
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    GLuint positionAttrib = glGetAttribLocation(prog, "position");
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *) 0);

    GLuint texCoordAttrib = glGetAttribLocation(prog, "texCoord");
    glEnableVertexAttribArray(texCoordAttrib);
    glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *) (2 * sizeof(GLfloat)));

    glUniform1i(glGetUniformLocation(prog, "textureY"), 0);
    glUniform1i(glGetUniformLocation(prog, "textureU"), 1);
    glUniform1i(glGetUniformLocation(prog, "textureV"), 2);

    while (!glfwWindowShouldClose(window)) {
        ret = av_read_frame(in_ctx, p);
        if (p->stream_index != s->index) {
            continue;
        }

        ret = avcodec_send_packet(cc, p);
        while (ret >= 0) {
            ret = avcodec_receive_frame(cc, f);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, f->width, f->height, 0, GL_RED, GL_UNSIGNED_BYTE, f->data[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textureU);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, f->width / 2, f->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, f->data[1]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, textureV);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, f->width / 2, f->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, f->data[2]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        av_packet_unref(p);
    }

    av_packet_free(&p);
    av_frame_free(&f);
    avcodec_free_context(&cc);
    avformat_close_input(&in_ctx);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
