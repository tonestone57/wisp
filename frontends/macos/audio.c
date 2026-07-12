#include <AudioToolbox/AudioToolbox.h>
#include <wisp/audio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

static AudioUnit outputUnit;
static uint8_t *audio_buffer = NULL;
static size_t audio_buffer_size = 0;
static size_t audio_buffer_cap = 0;
static pthread_mutex_t audio_lock = PTHREAD_MUTEX_INITIALIZER;

static OSStatus RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    pthread_mutex_lock(&audio_lock);
    for (UInt32 i = 0; i < ioData->mNumberBuffers; i++) {
        UInt32 bytesNeeded = ioData->mBuffers[i].mDataByteSize;
        if (audio_buffer && audio_buffer_size >= bytesNeeded) {
            memcpy(ioData->mBuffers[i].mData, audio_buffer, bytesNeeded);
            memmove(audio_buffer, audio_buffer + bytesNeeded, audio_buffer_size - bytesNeeded);
            audio_buffer_size -= bytesNeeded;
        } else {
            if (audio_buffer && audio_buffer_size > 0) {
                memcpy(ioData->mBuffers[i].mData, audio_buffer, audio_buffer_size);
                memset((uint8_t *)ioData->mBuffers[i].mData + audio_buffer_size, 0, bytesNeeded - audio_buffer_size);
                audio_buffer_size = 0;
            } else {
                memset(ioData->mBuffers[i].mData, 0, bytesNeeded);
            }
        }
    }
    pthread_mutex_unlock(&audio_lock);
    return noErr;
}

static bool macos_audio_init(int rate, int channels) {
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent comp = AudioComponentFindNext(NULL, &desc);
    AudioComponentInstanceNew(comp, &outputUnit);
    AudioUnitInitialize(outputUnit);

    AURenderCallbackStruct input;
    input.inputProc = RenderCallback;
    input.inputProcRefCon = NULL;
    AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &input, sizeof(input));

    AudioStreamBasicDescription format;
    format.mSampleRate = (Float64)rate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel = 32;
    format.mChannelsPerFrame = (UInt32)channels;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = 4 * (UInt32)channels;
    format.mBytesPerPacket = 4 * (UInt32)channels;

    AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format));
    AudioOutputUnitStart(outputUnit);
    return true;
}

static void macos_audio_play(const void *data, size_t size) {
    pthread_mutex_lock(&audio_lock);
    if (audio_buffer_size + size > audio_buffer_cap) {
        size_t new_cap = audio_buffer_cap * 2 + size + 4096;
        uint8_t *new_buf = realloc(audio_buffer, new_cap);
        if (new_buf) {
            audio_buffer = new_buf;
            audio_buffer_cap = new_cap;
        }
    }
    if (audio_buffer && audio_buffer_size + size <= audio_buffer_cap) {
        memcpy(audio_buffer + audio_buffer_size, data, size);
        audio_buffer_size += size;
    }
    pthread_mutex_unlock(&audio_lock);
}

static void macos_audio_fini(void) {
    AudioOutputUnitStop(outputUnit);
    AudioUnitUninitialize(outputUnit);
    AudioComponentInstanceDispose(outputUnit);

    pthread_mutex_lock(&audio_lock);
    if (audio_buffer) {
        free(audio_buffer);
        audio_buffer = NULL;
    }
    audio_buffer_size = 0;
    audio_buffer_cap = 0;
    pthread_mutex_unlock(&audio_lock);
}

struct gui_audio_table macos_audio_table_data = {
    .init = macos_audio_init,
    .play = macos_audio_play,
    .fini = macos_audio_fini,
};

struct gui_audio_table *macos_audio_table = &macos_audio_table_data;
