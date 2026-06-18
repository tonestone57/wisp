#include <SoundPlayer.h>
#include <stdbool.h>
#include <string.h>
extern "C" {
#include <wisp/audio.h>
}

static BSoundPlayer *player = NULL;

static bool beos_audio_init(int rate, int channels) {
    media_raw_audio_format format;
    format.frame_rate = rate;
    format.channel_count = channels;
    format.format = media_raw_audio_format::B_AUDIO_SHORT;
    format.byte_order = B_MEDIA_LITTLE_ENDIAN;
    format.buffer_size = 4096;

    player = new BSoundPlayer(&format, "Wisp Audio");
    if (player->InitCheck() != B_OK) {
        delete player;
        player = NULL;
        return false;
    }
    player->Start();
    return true;
}

static void beos_audio_play(const void *data, size_t size) {
}

static void beos_audio_fini(void) {
    if (player) {
        player->Stop();
        delete player;
        player = NULL;
    }
}

static struct gui_audio_table audio_table = {
    .init = beos_audio_init,
    .play = beos_audio_play,
    .fini = beos_audio_fini,
};

extern "C" {
struct gui_audio_table *beos_audio_table = &audio_table;
}
