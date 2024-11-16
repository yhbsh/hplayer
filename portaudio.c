#include <portaudio/portaudio.h>

#include <stdio.h>
#include <math.h>
#include <stdint.h>

static int callback(const void *input, void *output, unsigned long frameCount, 
                    const PaStreamCallbackTimeInfo *timeInfo, 
                    PaStreamCallbackFlags statusFlags, void *in) {
    int16_t *out = (int16_t *)output; // Use int16_t for 16-bit PCM
    double *phase = (double *)in;
    for (size_t i = 0; i < frameCount; i++) {
        *out++ = (int16_t)(32767 * 0.5 * sin(*phase)); // Scale to 16-bit range
        *phase += 2.0 * M_PI * 440.0 / 44100.0; // Update phase
        if (*phase >= 2.0 * M_PI) { 
            *phase -= 2.0 * M_PI; // Keep phase within 0 to 2π
        }
    }
    return paContinue;
}

int main() {
    PaStream *stream;
    double phase = 0.0;

    Pa_Initialize();
    Pa_OpenDefaultStream(&stream, 0, 1, paInt16, 44100, 256, callback, &phase); // Use paInt16 format
    Pa_StartStream(stream);
    while (1); // Infinite loop to keep the stream running
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}
