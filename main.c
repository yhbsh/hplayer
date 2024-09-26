#include <stdio.h>

#include "pl.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[USAGE]: ./main <url>\n");
        return 1;
    }

    int ret;
    PL_Engine *pl_engine = NULL;

    if ((ret = pl_engine_init(&pl_engine, argv[1])) < 0) {
        fprintf(stderr, "[ERROR]: pl_engine_init(): %s\n", pl_err2str(ret));
        return 1;
    }

    while (!pl_window_should_close(pl_engine)) {
        ret = pl_read_packet(pl_engine);

        if (ret == PL_ERROR_WOULD_BLOCK) continue;
        if (ret == PL_ERROR_EOF) break;

        if (pl_engine_is_vpacket(pl_engine)) {
            if ((ret = pl_send_vpacket(pl_engine)) < 0) {
                fprintf(stderr, "[ERROR]: pl_send_packet(): %s\n", pl_err2str(ret));
                return 1;
            }

            while (ret >= 0) {
                ret = pl_receive_vframe(pl_engine);
                if (ret == PL_ERROR_WOULD_BLOCK || ret == PL_ERROR_EOF) break;
                if (ret < 0) {
                    fprintf(stderr, "[ERROR]: pl_receive_vframe(): %s\n", pl_err2str(ret));
                    return 1;
                }

                pl_delay_pts(pl_engine);
                pl_render_frame(pl_engine);
            }
        }

        pl_packet_unref(pl_engine);
    }

    pl_engine_deinit(&pl_engine);

    return 0;
}
