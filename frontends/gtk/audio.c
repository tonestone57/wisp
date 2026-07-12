#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <wisp/audio.h>

#ifdef WITH_PIPEWIRE

#include <pthread.h>
#include <spa/param/audio/format-utils.h>
#include <spa/utils/ringbuffer.h>
#include <pipewire/pipewire.h>

struct nsgtk_audio_state {
    struct pw_thread_loop *thread_loop;
    struct pw_stream *stream;

    pthread_mutex_t lock;
    pthread_cond_t cond;

    struct spa_ringbuffer ring;
    uint8_t *buffer;
    size_t buffer_size_frames;

    int rate;
    int channels;
    int stride;
    bool running;
};

static struct nsgtk_audio_state state = { 0 };

static void on_process(void *userdata) {
    struct nsgtk_audio_state *s = userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;
    uint8_t *p;

    if ((b = pw_stream_dequeue_buffer(s->stream)) == NULL) {
        return;
    }

    buf = b->buffer;
    if ((p = buf->datas[0].data) == NULL) {
        return;
    }

    pthread_mutex_lock(&s->lock);

    uint32_t index;
    int32_t filled = spa_ringbuffer_get_read_index(&s->ring, &index);

    int32_t n_frames = buf->datas[0].maxsize / s->stride;
    if (b->requested) {
        n_frames = SPA_MIN((int32_t)b->requested, n_frames);
    }

    int32_t to_read = filled > 0 ? SPA_MIN(filled, n_frames) : 0;
    int32_t to_silence = n_frames - to_read;

    if (to_read > 0) {
        spa_ringbuffer_read_data(&s->ring,
            s->buffer, s->buffer_size_frames * s->stride,
            (index % s->buffer_size_frames) * s->stride,
            p, to_read * s->stride);

        spa_ringbuffer_read_update(&s->ring, index + to_read);
    }

    if (to_silence > 0) {
        memset(SPA_PTROFF(p, to_read * s->stride, void), 0, to_silence * s->stride);
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = s->stride;
    buf->datas[0].chunk->size = n_frames * s->stride;

    pw_stream_queue_buffer(s->stream, b);

    pthread_cond_signal(&s->cond);

    pthread_mutex_unlock(&s->lock);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

static void nsgtk_audio_fini_pw(void);

static bool nsgtk_audio_init(int rate, int channels) {
    if (state.running) {
        nsgtk_audio_fini_pw();
    }

    state.rate = rate;
    state.channels = channels;
    state.stride = sizeof(float) * channels;

    pthread_mutex_init(&state.lock, NULL);
    pthread_cond_init(&state.cond, NULL);

    state.buffer_size_frames = 32768; // must be a power of two
    state.buffer = malloc(state.buffer_size_frames * state.stride);
    if (!state.buffer) {
        return false;
    }
    memset(state.buffer, 0, state.buffer_size_frames * state.stride);

    spa_ringbuffer_init(&state.ring);

    pw_init(NULL, NULL);

    state.thread_loop = pw_thread_loop_new("Wisp Audio", NULL);
    if (!state.thread_loop) {
        free(state.buffer);
        state.buffer = NULL;
        return false;
    }

    struct pw_loop *loop = pw_thread_loop_get_loop(state.thread_loop);

    if (pw_thread_loop_start(state.thread_loop) < 0) {
        pw_thread_loop_destroy(state.thread_loop);
        state.thread_loop = NULL;
        free(state.buffer);
        state.buffer = NULL;
        return false;
    }

    pw_thread_loop_lock(state.thread_loop);

    struct pw_properties *props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Movie",
        NULL
    );

    state.stream = pw_stream_new_simple(
        loop,
        "Wisp Playback",
        props,
        &stream_events,
        &state
    );

    if (!state.stream) {
        pw_thread_loop_unlock(state.thread_loop);
        pw_thread_loop_destroy(state.thread_loop);
        state.thread_loop = NULL;
        free(state.buffer);
        state.buffer = NULL;
        return false;
    }

    uint8_t format_buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(format_buffer, sizeof(format_buffer));
    const struct spa_pod *params[1];
    params[0] = spa_format_audio_raw_build(&b,
        SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_F32,
            .channels = channels,
            .rate = rate
        )
    );

    int res = pw_stream_connect(state.stream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        PW_STREAM_FLAG_AUTOCONNECT |
        PW_STREAM_FLAG_MAP_BUFFERS |
        PW_STREAM_FLAG_RT_PROCESS,
        params, 1);

    if (res < 0) {
        pw_stream_destroy(state.stream);
        state.stream = NULL;
        pw_thread_loop_unlock(state.thread_loop);
        pw_thread_loop_destroy(state.thread_loop);
        state.thread_loop = NULL;
        free(state.buffer);
        state.buffer = NULL;
        return false;
    }

    state.running = true;
    pw_thread_loop_unlock(state.thread_loop);

    return true;
}

static void nsgtk_audio_play(const void *data, size_t size) {
    pthread_mutex_lock(&state.lock);
    if (!state.running) {
        pthread_mutex_unlock(&state.lock);
        return;
    }

    int32_t n_frames = size / state.stride;
    const uint8_t *data_ptr = data;

    while (n_frames > 0 && state.running) {
        uint32_t write_index;
        int32_t filled = spa_ringbuffer_get_write_index(&state.ring, &write_index);
        int32_t avail = state.buffer_size_frames - filled;

        if (avail <= 0) {
            pthread_cond_wait(&state.cond, &state.lock);
            continue;
        }

        int32_t to_write = SPA_MIN(avail, n_frames);
        spa_ringbuffer_write_data(&state.ring,
            state.buffer, state.buffer_size_frames * state.stride,
            (write_index % state.buffer_size_frames) * state.stride,
            data_ptr, to_write * state.stride);

        spa_ringbuffer_write_update(&state.ring, write_index + to_write);

        data_ptr += to_write * state.stride;
        n_frames -= to_write;
    }

    pthread_mutex_unlock(&state.lock);
}

static void nsgtk_audio_fini_pw(void) {
    if (!state.running) return;

    pthread_mutex_lock(&state.lock);
    state.running = false;
    pthread_cond_broadcast(&state.cond);
    pthread_mutex_unlock(&state.lock);

    if (state.thread_loop) {
        pw_thread_loop_stop(state.thread_loop);
    }

    if (state.stream) {
        pw_thread_loop_lock(state.thread_loop);
        pw_stream_destroy(state.stream);
        state.stream = NULL;
        pw_thread_loop_unlock(state.thread_loop);
    }

    if (state.thread_loop) {
        pw_thread_loop_destroy(state.thread_loop);
        state.thread_loop = NULL;
    }

    pthread_mutex_destroy(&state.lock);
    pthread_cond_destroy(&state.cond);

    if (state.buffer) {
        free(state.buffer);
        state.buffer = NULL;
    }

    pw_deinit();
}

static void nsgtk_audio_fini(void) {
    nsgtk_audio_fini_pw();
}

#elif defined(WITH_PULSE)

#include <pulse/simple.h>
#include <pulse/error.h>

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

#else /* Silent dummy fallback */

static bool nsgtk_audio_init(int rate, int channels) {
    (void)rate;
    (void)channels;
    return true;
}

static void nsgtk_audio_play(const void *data, size_t size) {
    (void)data;
    (void)size;
}

static void nsgtk_audio_fini(void) {
}

#endif

static struct gui_audio_table audio_table = {
    .init = nsgtk_audio_init,
    .play = nsgtk_audio_play,
    .fini = nsgtk_audio_fini,
};

struct gui_audio_table *nsgtk_audio_table = &audio_table;
