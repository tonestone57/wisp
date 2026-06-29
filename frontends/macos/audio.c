#include <AudioToolbox/AudioToolbox.h>
#include <wisp/audio.h>
#include <string.h>

static AudioUnit outputUnit;

static OSStatus RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    /* Feed silent for now */
    for (UInt32 i = 0; i < ioData->mNumberBuffers; i++) {
        memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
    }
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
    /* AudioUnit uses a pull-model callback, we would buffer here. */
}

static void macos_audio_fini(void) {
    AudioOutputUnitStop(outputUnit);
    AudioUnitUninitialize(outputUnit);
    AudioComponentInstanceDispose(outputUnit);
}

struct gui_audio_table macos_audio_table_data = {
    .init = macos_audio_init,
    .play = macos_audio_play,
    .fini = macos_audio_fini,
};

struct gui_audio_table *macos_audio_table = &macos_audio_table_data;
