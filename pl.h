#ifndef PL_H
#define PL_H

#define PL_API

typedef struct PL_Engine PL_Engine;

#define PL_ERROR_EOF -1
#define PL_ERROR_WOULD_BLOCK -2

/* Engine */
PL_API int pl_engine_init(PL_Engine **pl_engine, const char *url);
PL_API void pl_engine_deinit(PL_Engine **pl_engine);

/* Window */
PL_API int pl_window_should_close(PL_Engine *pl_engine);

/* Video */
PL_API int pl_engine_is_vpacket(PL_Engine *pl_engine);
PL_API int pl_send_vpacket(PL_Engine *pl_engine);
PL_API int pl_receive_vframe(PL_Engine *pl_engine);

/* Audio */
PL_API int pl_engine_is_apacket(PL_Engine *pl_engine);
PL_API int pl_send_apacket(PL_Engine *pl_engine);
PL_API int pl_receive_aframe(PL_Engine *pl_engine);

PL_API int pl_read_packet(PL_Engine *pl_engine);
PL_API void pl_packet_unref(PL_Engine *pl_engine);

PL_API void pl_render_frame(PL_Engine *pl_engine);
PL_API void pl_delay_pts(PL_Engine *pl_engine);

PL_API const char *pl_err2str(int code);

#endif // PL_H
