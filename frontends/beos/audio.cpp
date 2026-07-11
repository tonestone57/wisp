#include <SoundPlayer.h>
#include <stdbool.h>
#include <string.h>
#include <vector>
#include <pthread.h>

extern "C" {
#include <wisp/audio.h>
}

static BSoundPlayer *player = NULL;
static std::vector<uint8_t> audio_buffer;
static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;

static void audio_callback(void *cookie, void *buffer, size_t size, const media_raw_audio_format &format) {
    pthread_mutex_lock(&buffer_lock);
    if (audio_buffer.size() >= size) {
        memcpy(buffer, audio_buffer.data(), size);
        audio_buffer.erase(audio_buffer.begin(), audio_buffer.begin() + size);
    } else {
        memset(buffer, 0, size);
    }
    pthread_mutex_unlock(&buffer_lock);
}

static void beos_audio_fini(void);

static bool beos_audio_init(int rate, int channels) {
    beos_audio_fini(); /* Ensure any existing player/buffers are completely cleaned up first */

    media_raw_audio_format format;
    format.frame_rate = (float)rate;
    format.channel_count = (uint32)channels;
    format.format = media_raw_audio_format::B_AUDIO_FLOAT;
    format.byte_order = B_MEDIA_LITTLE_ENDIAN;
    format.buffer_size = 4096;

    player = new BSoundPlayer(&format, "Wisp Audio", audio_callback);
    if (player->InitCheck() != B_OK) {
        delete player;
        player = NULL;
        return false;
    }
    player->Start();
    player->SetHasData(true);
    return true;
}

static void beos_audio_play(const void *data, size_t size) {
    pthread_mutex_lock(&buffer_lock);
    audio_buffer.insert(audio_buffer.end(), (uint8_t *)data, (uint8_t *)data + size);
    pthread_mutex_unlock(&buffer_lock);
}

static void beos_audio_fini(void) {
    if (player) {
        player->Stop();
        delete player;
        player = NULL;
    }
    pthread_mutex_lock(&buffer_lock);
    audio_buffer.clear();
    pthread_mutex_unlock(&buffer_lock);
}

static struct gui_audio_table audio_table = {
    .init = beos_audio_init,
    .play = beos_audio_play,
    .fini = beos_audio_fini,
};

extern "C" {
struct gui_audio_table *beos_audio_table = &audio_table;
}
