#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdbool.h>
#include <initguid.h>
#include <wisp/audio.h>

static IAudioClient *pAudioClient = NULL;
static IAudioRenderClient *pRenderClient = NULL;

static bool win32_audio_init(int rate, int channels) {
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    WAVEFORMATEX *pwfx = NULL;

    CoInitialize(NULL);

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) return false;

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    pEnumerator->Release();
    if (FAILED(hr)) return false;

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    pDevice->Release();
    if (FAILED(hr)) return false;

    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) return false;

    if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE *pEwfx = (WAVEFORMATEXTENSIBLE*)pwfx;
        pEwfx->Format.nSamplesPerSec = rate;
        pEwfx->Format.nChannels = channels;
        pEwfx->Format.wBitsPerSample = 32;
        pEwfx->Format.nBlockAlign = (pEwfx->Format.wBitsPerSample * pEwfx->Format.nChannels) / 8;
        pEwfx->Format.nAvgBytesPerSec = pEwfx->Format.nSamplesPerSec * pEwfx->Format.nBlockAlign;
        pEwfx->Samples.wValidBitsPerSample = 32;
        pEwfx->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    }

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, pwfx, NULL);
    if (FAILED(hr)) return false;

    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient);
    if (FAILED(hr)) return false;

    hr = pAudioClient->Start();
    return SUCCEEDED(hr);
}

static void win32_audio_play(const void *data, size_t size) {
    if (!pRenderClient) return;

    UINT32 bufferFrameCount;
    UINT32 numFramesPadding;
    BYTE *pData;

    pAudioClient->GetBufferSize(&bufferFrameCount);
    pAudioClient->GetCurrentPadding(&numFramesPadding);

    UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
    UINT32 framesToWrite = size / 8; // 4 bytes per sample * 2 channels
    if (framesToWrite > numFramesAvailable) framesToWrite = numFramesAvailable;

    if (framesToWrite > 0) {
        pRenderClient->GetBuffer(framesToWrite, &pData);
        memcpy(pData, data, framesToWrite * 8);
        pRenderClient->ReleaseBuffer(framesToWrite, 0);
    }
}

static void win32_audio_fini(void) {
    if (pAudioClient) {
        pAudioClient->Stop();
        pAudioClient->Release();
        pAudioClient = NULL;
    }
    if (pRenderClient) {
        pRenderClient->Release();
        pRenderClient = NULL;
    }
    CoUninitialize();
}

static struct gui_audio_table audio_table = {
    .init = win32_audio_init,
    .play = win32_audio_play,
    .fini = win32_audio_fini,
};

struct gui_audio_table *win32_audio_table = &audio_table;
