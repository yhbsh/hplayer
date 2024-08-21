#include <stdio.h>

#include "pl.h"

void draw_frame(PL_Engine *pl_engine) {
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
}

void delay_pts(PL_Engine *pl_engine) {
    int64_t pts = pl_engine->frame->best_effort_timestamp;
    if (pts == AV_NOPTS_VALUE) pts = 0;
    pl_engine->pts_time  = av_rescale_q(pts, pl_engine->video_stream->time_base, AV_TIME_BASE_Q);
    int64_t current_time = av_gettime_relative() - pl_engine->start_time;
    int64_t delay        = (pl_engine->pts_time - current_time);

    if (delay > 0) av_usleep(delay);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[USAGE]: ./main <file.mp4>\n");
        return 1;
    }

    int ret;
    PL_Engine *pl_engine = NULL;

    if ((ret = pl_engine_init(&pl_engine, argv[1])) < 0) {
        fprintf(stderr, "[ERROR]: init_engine(): %s\n", av_err2str(ret));
        return 1;
    }

    while (glfwWindowShouldClose(pl_engine->window) == 0) {
        ret = av_read_frame(pl_engine->format_context, pl_engine->packet);

        if (ret == AVERROR(EAGAIN)) continue;
        if (ret == AVERROR_EOF) break;

        /* Non-Video Frame */
        if (pl_engine->packet->stream_index != pl_engine->video_stream->index) continue;

        ret = avcodec_send_packet(pl_engine->video_codec_context, pl_engine->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) continue;

        if (ret < 0) {
            fprintf(stderr, "[ERROR]: avcodec_send_packet(): %s\n", av_err2str(ret));
            return 1;
        };

        while (ret >= 0) {
            ret = avcodec_receive_frame(pl_engine->video_codec_context, pl_engine->frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

            delay_pts(pl_engine);
            draw_frame(pl_engine);

            glfwPollEvents();
            glfwSwapBuffers(pl_engine->window);
        }

        av_packet_unref(pl_engine->packet);
    }

    pl_engine_deinit(&pl_engine);

    return 0;
}
