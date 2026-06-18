#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdbool.h>
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

    pwfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    pwfx->nSamplesPerSec = rate;
    pwfx->nChannels = channels;
    pwfx->wBitsPerSample = 32;
    pwfx->nBlockAlign = (pwfx->wBitsPerSample * pwfx->nChannels) / 8;
    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;

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
    pRenderClient->GetBuffer(numFramesAvailable, &pData);

    // Copy data to pData (simplified)
    memcpy(pData, data, size < numFramesAvailable * 4 ? size : numFramesAvailable * 4);

    pRenderClient->ReleaseBuffer(numFramesAvailable, 0);
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
