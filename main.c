#include <stdio.h>

#include "ffplay.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[USAGE]: ./main <file.mp4>\n");
        return 1;
    }

    int ret;
    PlayerEngine *p_engine = NULL;

    if ((ret = init_engine(&p_engine, argv[1])) < 0) {
        fprintf(stderr, "[ERROR]: init_engine(): %s\n", av_err2str(ret));
        return 1;
    }

    while (glfwWindowShouldClose(p_engine->window) == 0) {
        ret = av_read_frame(p_engine->format_context, p_engine->packet);

        if (ret == AVERROR(EAGAIN)) continue;
        if (ret == AVERROR_EOF) break;

        if (p_engine->video_stream && p_engine->packet->stream_index == p_engine->video_stream->index) {
            if ((ret = avcodec_send_packet(p_engine->video_codec_context, p_engine->packet)) < 0) {
                fprintf(stderr, "[ERROR]: avcodec_send_packet(): %s\n", av_err2str(ret));
                return 1;
            };

            do {
                ret = avcodec_receive_frame(p_engine->video_codec_context, p_engine->frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

                glClear(GL_COLOR_BUFFER_BIT);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, p_engine->textures[0]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, p_engine->frame->width, p_engine->frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, p_engine->frame->data[0]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, p_engine->textures[1]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, p_engine->frame->width / 2, p_engine->frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, p_engine->frame->data[1]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, p_engine->textures[2]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, p_engine->frame->width / 2, p_engine->frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, p_engine->frame->data[2]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                glfwPollEvents();
                glfwSwapBuffers(p_engine->window);
            } while (ret >= 0);
        }

        av_packet_unref(p_engine->packet);
    }

    deinit_engine(&p_engine);

    return 0;
}
