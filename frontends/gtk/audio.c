#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <wisp/audio.h>

static pa_simple *pa_s = NULL;

static bool nsgtk_audio_init(int rate, int channels) {
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = rate;
    ss.channels = channels;

    int error;
    pa_s = pa_simple_new(NULL, "Wisp", PA_STREAM_PLAYBACK, NULL, "Video Playback", &ss, NULL, NULL, &error);
    if (!pa_s) {
        fprintf(stderr, "PulseAudio: pa_simple_new() failed: %s\n", pa_strerror(error));
        return false;
    }
    return true;
}

static void nsgtk_audio_play(const void *data, size_t size) {
    if (!pa_s) return;
    int error;
    if (pa_simple_write(pa_s, data, size, &error) < 0) {
        fprintf(stderr, "PulseAudio: pa_simple_write() failed: %s\n", pa_strerror(error));
    }
}

static void nsgtk_audio_fini(void) {
    if (pa_s) {
        pa_simple_drain(pa_s, NULL);
        pa_simple_free(pa_s);
        pa_s = NULL;
    }
}

static struct gui_audio_table audio_table = {
    .init = nsgtk_audio_init,
    .play = nsgtk_audio_play,
    .fini = nsgtk_audio_fini,
};

struct gui_audio_table *nsgtk_audio_table = &audio_table;
