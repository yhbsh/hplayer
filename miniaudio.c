#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <stdio.h>
#include <math.h>

#define SAMPLE_RATE 48000
#define CHANNELS 1

void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float* pOutputF32 = (float*)pOutput;
    float* phase = (float*)pDevice->pUserData;

    printf("frames %d\n", frameCount);
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        pOutputF32[i] = sinf(*phase);
        *phase += 2.0f * M_PI * 440.0f / SAMPLE_RATE; // Frequency: 440 Hz
        if (*phase >= 2.0f * M_PI) {
            *phase -= 2.0f * M_PI;
        }
    }
}

int main() {
    ma_device_config config;
    ma_device device;
    float phase = 0.0f;

    config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = CHANNELS;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = audio_callback;
    config.pUserData = &phase;

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        fprintf(stderr, "ERROR: ma_device_init\n");
        return -1;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        fprintf(stderr, "ERROR: ma_device_start");
        ma_device_uninit(&device);
        return -2;
    }

    while(1);

    ma_device_uninit(&device);

    return 0;
}
