#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

const char *vs =
    "#version 410\n"
    "layout (location = 0) in vec4 position;\n"
    "layout (location = 1) in vec2 texCoord;\n"
    "out vec2 texCoordVarying;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "    texCoordVarying = texCoord;\n"
    "}\n";

const char *fs =
    "#version 410\n"
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
    fprintf(stderr, "[USAGE]: ./main [url]");
    return 1;
  }

  int ret;
  const int width = 960;
  const int height = 540;

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow *window = glfwCreateWindow(width, height, "Animated UV Pattern", NULL, NULL);
  glfwMakeContextCurrent(window);
  glewInit();
  printf("renderer = %s\nversion = %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

  AVFormatContext *format_context = NULL;
  ret = avformat_open_input(&format_context, argv[1], NULL, NULL);
  ret = avformat_find_stream_info(format_context, NULL);
  AVStream *stream = format_context->streams[1];
  enum AVCodecID codec_id = stream->codecpar->codec_id;
  const AVCodec *codec = avcodec_find_decoder(codec_id);
  AVCodecContext *codec_context = avcodec_alloc_context3(codec);
  ret = avcodec_parameters_to_context(codec_context, stream->codecpar);
  ret = avcodec_open2(codec_context, codec, NULL);
  AVFrame *frame = av_frame_alloc();
  AVPacket packet;

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vs, NULL);
  glCompileShader(vertex_shader);
  GLint status;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fs, NULL);
  glCompileShader(fragment_shader);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  glUseProgram(program);

  GLuint textureY, textureU, textureV;
  glGenTextures(1, &textureY);
  glGenTextures(1, &textureV);
  glGenTextures(1, &textureU);

  GLfloat vertices[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat texCoords[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};

  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(texCoords), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(texCoords), texCoords);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)sizeof(vertices));
  glEnableVertexAttribArray(1);

  while (!glfwWindowShouldClose(window)) {
    ret = av_read_frame(format_context, &packet);
    if (packet.stream_index != stream->index) {
      continue;
    }

    ret = avcodec_send_packet(codec_context, &packet);
    while (ret >= 0) {
      ret = avcodec_receive_frame(codec_context, frame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      }

      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      // upload to gpu
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textureY);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width, frame->height, 0, GL_RED,
                   GL_UNSIGNED_BYTE, frame->data[0]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glUniform1i(glGetUniformLocation(program, "textureY"), 0);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, textureU);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED,
                   GL_UNSIGNED_BYTE, frame->data[1]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glUniform1i(glGetUniformLocation(program, "textureU"), 1);

      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, textureV);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED,
                   GL_UNSIGNED_BYTE, frame->data[2]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glUniform1i(glGetUniformLocation(program, "textureV"), 2);

      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }

    av_packet_unref(&packet);
  }

  glfwTerminate();
  return 0;
}
