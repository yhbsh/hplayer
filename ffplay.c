#include "ffplay.h"

int init_ffmpeg(PlayerEngine *p_engine, const char *url) {
    int ret;
    av_log_set_level(AV_LOG_DEBUG);

    if ((ret = avformat_open_input(&p_engine->format_context, url, NULL, NULL)) < 0) return ret;
    if ((ret = avformat_find_stream_info(p_engine->format_context, NULL)) < 0) return ret;

    const AVCodec *video_codec = NULL;
    const AVCodec *audio_codec = NULL;

    ret = av_find_best_stream(p_engine->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    if (ret >= 0) {
        if (!(p_engine->video_stream = p_engine->format_context->streams[ret])) return AVERROR(ENOMEM);
        if (!(p_engine->video_codec_context = avcodec_alloc_context3(video_codec))) return AVERROR(ENOMEM);
        if ((ret = avcodec_parameters_to_context(p_engine->video_codec_context, p_engine->video_stream->codecpar)) < 0) return ret;
        if ((ret = avcodec_open2(p_engine->video_codec_context, video_codec, NULL)) < 0) return ret;
    }

    ret = av_find_best_stream(p_engine->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &audio_codec, 0);
    if (ret >= 0) {
        if (!(p_engine->audio_stream = p_engine->format_context->streams[ret])) return AVERROR(ENOMEM);
        if (!(p_engine->audio_codec_context = avcodec_alloc_context3(video_codec))) return AVERROR(ENOMEM);
        if ((ret = avcodec_parameters_to_context(p_engine->audio_codec_context, p_engine->audio_stream->codecpar)) < 0) return ret;
        if ((ret = avcodec_open2(p_engine->audio_codec_context, video_codec, NULL)) < 0) return ret;
    }

    if (!(p_engine->packet = av_packet_alloc())) return AVERROR(ENOMEM);
    if (!(p_engine->frame = av_frame_alloc())) return AVERROR(ENOMEM);

    return 0;
}

void deinit_ffmpeg(PlayerEngine *p_engine) {
    av_packet_free(&p_engine->packet);
    av_frame_free(&p_engine->frame);
    avcodec_free_context(&p_engine->video_codec_context);
    avcodec_free_context(&p_engine->audio_codec_context);
    avformat_close_input(&p_engine->format_context);
}

void error_callback(int error, const char *description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
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

int init_glfw(PlayerEngine *p_engine) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) return -1;

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

    if (!(p_engine->window = glfwCreateWindow(width, height, "FFplay", NULL, NULL))) return -1;

    glfwSetKeyCallback(p_engine->window, key_callback);
    glfwSetFramebufferSizeCallback(p_engine->window, framebuffer_size_callback);

    glfwMakeContextCurrent(p_engine->window);
    glfwSwapInterval(0);

    glfwSetWindowPos(p_engine->window, (mode->width - width) / 2, (mode->height - height) / 2);

    return 0;
}

void deinit_glfw(PlayerEngine *p_engine) {
    if (p_engine->window) glfwDestroyWindow(p_engine->window);
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

int init_opengl(PlayerEngine *p_engine) {
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *version     = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

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

    p_engine->prog = glCreateProgram();
    glAttachShader(p_engine->prog, vert);
    glAttachShader(p_engine->prog, frag);
    glLinkProgram(p_engine->prog);

    glGetProgramiv(p_engine->prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(p_engine->prog, 512, NULL, infoLog);
        printf("Shader Program Linking Failed: %s\n", infoLog);
        return -1;
    }

    glUseProgram(p_engine->prog);

    glGenTextures(3, p_engine->textures);

    // clang-format off
    GLfloat vertices[] = {
        -1.0f, +1.0f, +0.0f, +0.0f,
        -1.0f, -1.0f, +0.0f, +1.0f,
        +1.0f, +1.0f, +1.0f, +0.0f,
        +1.0f, -1.0f, +1.0f, +1.0f,
    };
    // clang-format on

    glGenVertexArrays(1, &p_engine->vao);
    glBindVertexArray(p_engine->vao);

    glGenBuffers(1, &p_engine->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p_engine->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint positionAttrib = glGetAttribLocation(p_engine->prog, "position");
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);

    GLuint texCoordAttrib = glGetAttribLocation(p_engine->prog, "texCoord");
    glEnableVertexAttribArray(texCoordAttrib);
    glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));

    glUniform1i(glGetUniformLocation(p_engine->prog, "textureY"), 0);
    glUniform1i(glGetUniformLocation(p_engine->prog, "textureU"), 1);
    glUniform1i(glGetUniformLocation(p_engine->prog, "textureV"), 2);

    // Clean up shaders
    glDetachShader(p_engine->prog, vert);
    glDetachShader(p_engine->prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    return 0;
}

void deinit_opengl(PlayerEngine *p_engine) {
    glDeleteProgram(p_engine->prog);
    glDeleteVertexArrays(1, &p_engine->vao);
    glDeleteBuffers(1, &p_engine->vbo);
    glDeleteTextures(3, p_engine->textures);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

int init_engine(PlayerEngine **o_p_engine, const char *url) {
    int ret;

    PlayerEngine *p_engine = (PlayerEngine *)av_calloc(1, sizeof(PlayerEngine));

    if ((ret = init_ffmpeg(p_engine, url)) < 0) return ret;
    if ((ret = init_glfw(p_engine)) < 0) return ret;
    if ((ret = init_opengl(p_engine)) < 0) return ret;

    *o_p_engine = p_engine;

    return 0;
}
void deinit_engine(PlayerEngine **p_engine) {
    deinit_ffmpeg(*p_engine);
    deinit_glfw(*p_engine);
    deinit_opengl(*p_engine);

    free(*p_engine);
    *p_engine = NULL;
}
