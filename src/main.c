#include "ffmpeg.h"
#include "glfw.h"
#include "opengl.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[USAGE]: ./main <file.mp4>\n");
        return 1;
    }

    int ret;
    GLFWwindow *window                  = NULL;
    AVFormatContext *format_context     = NULL;
    const AVCodec *video_codec          = NULL;
    const AVCodec *audio_codec          = NULL;
    AVCodecContext *video_codec_context = NULL;
    AVCodecContext *audio_codec_context = NULL;
    AVStream *video_stream              = NULL;
    AVStream *audio_stream              = NULL;
    AVPacket *packet                    = NULL;
    AVFrame *frame                      = NULL;
    GLuint program, vao, vbo, textures[3];

    ret = init_ffmpeg(argv[1], &format_context, &video_stream, &audio_stream, &video_codec_context, &audio_codec_context, &packet, &frame);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: init_ffmpeg(): %s\n", av_err2str(ret));
        return 1;
    }

    ret = init_glfw(&window);
    if (ret < 0) {
        return 1;
    }

    ret = init_opengl(&program, &vao, &vbo, textures);
    if (ret < 0) {
        return 1;
    }

    while (glfwWindowShouldClose(window) == 0) {
        if (av_read_frame(format_context, packet) < 0) {
            break;
        }

        if (video_stream && packet->stream_index == video_stream->index) {
            ret = avcodec_send_packet(video_codec_context, packet);
            if (ret < 0) {
                break;
            }

            do {
                ret = avcodec_receive_frame(video_codec_context, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }

                glClear(GL_COLOR_BUFFER_BIT);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textures[0]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width, frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, textures[1]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, textures[2]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                glfwPollEvents();
                glfwSwapBuffers(window);
            } while (ret >= 0);
        }

        av_packet_unref(packet);
    }

    deinit_opengl(program, vao, vbo, textures);
    deinit_glfw(window);
    deinit_ffmpeg(&format_context, &video_codec_context, &audio_codec_context, &packet, &frame);

    return 0;
}
