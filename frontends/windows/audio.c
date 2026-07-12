#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdbool.h>
#include <initguid.h>
#include <wisp/audio.h>

static IAudioClient *pAudioClient = NULL;
static IAudioRenderClient *pRenderClient = NULL;
static int audio_bytes_per_frame = 8;

static bool win32_audio_init(int rate, int channels) {
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    WAVEFORMATEX *pwfx = NULL;
    bool success = false;

    CoInitialize(NULL);

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) goto cleanup;

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    pEnumerator->Release();
    pEnumerator = NULL;
    if (FAILED(hr)) goto cleanup;

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    pDevice->Release();
    pDevice = NULL;
    if (FAILED(hr)) goto cleanup;

    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) goto cleanup;

    if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE *pEwfx = (WAVEFORMATEXTENSIBLE*)pwfx;
        pEwfx->Format.nSamplesPerSec = rate;
        pEwfx->Format.nChannels = channels;
        pEwfx->Format.wBitsPerSample = 32;
        pEwfx->Format.nBlockAlign = (pEwfx->Format.wBitsPerSample * pEwfx->Format.nChannels) / 8;
        pEwfx->Format.nAvgBytesPerSec = pEwfx->Format.nSamplesPerSec * pEwfx->Format.nBlockAlign;
        pEwfx->Samples.wValidBitsPerSample = 32;
        pEwfx->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        audio_bytes_per_frame = pEwfx->Format.nBlockAlign;
    } else {
        audio_bytes_per_frame = pwfx->nBlockAlign;
    }

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, pwfx, NULL);
    if (FAILED(hr)) goto cleanup;

    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient);
    if (FAILED(hr)) goto cleanup;

    hr = pAudioClient->Start();
    if (SUCCEEDED(hr)) {
        success = true;
    }

cleanup:
    if (pEnumerator) pEnumerator->Release();
    if (pDevice) pDevice->Release();
    if (pwfx) CoTaskMemFree(pwfx);
    if (!success) {
        if (pAudioClient) {
            pAudioClient->Release();
            pAudioClient = NULL;
        }
        if (pRenderClient) {
            pRenderClient->Release();
            pRenderClient = NULL;
        }
        CoUninitialize();
    }
    return success;
}

static void win32_audio_play(const void *data, size_t size) {
    if (!pRenderClient) return;

    UINT32 bufferFrameCount;
    UINT32 numFramesPadding;
    BYTE *pData;

    pAudioClient->GetBufferSize(&bufferFrameCount);
    pAudioClient->GetCurrentPadding(&numFramesPadding);

    UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
    UINT32 framesToWrite = size / audio_bytes_per_frame;
    if (framesToWrite > numFramesAvailable) framesToWrite = numFramesAvailable;

    if (framesToWrite > 0) {
        pRenderClient->GetBuffer(framesToWrite, &pData);
        memcpy(pData, data, framesToWrite * audio_bytes_per_frame);
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
