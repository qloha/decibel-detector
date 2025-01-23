#include <iostream>
#include <cmath>
#include <vector>
#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <atomic>
#include <conio.h>
#pragma comment(lib, "winmm.lib")

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define BITS_PER_SAMPLE 16
#define BUFFER_SIZE 44100

std::atomic<bool> running(true);
std::atomic<bool> paused(false);

double calculateRMS(const std::vector<int>& samples) {
    double sum = 0.0;
    for (int sample : samples) {
        sum += sample * sample;
    }
    return std::sqrt(sum / samples.size());
}

double rmsToDecibels(double rms) {
    const double reference = 1.0;
    if (rms == 0) {
        return -std::numeric_limits<double>::infinity();
    }
    return 20 * std::log10(rms / reference);
}

void handleInput() {
    while (running) {
        int ch = _getch();
        if (ch == 27) {
            running = false;
        } else if (ch == 13) {
            paused = !paused;
            if (paused) {
                std::cout << "\rPAUSED                         " << std::flush;
            }
        }
    }
}

void CALLBACK waveInProc(
    HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (uMsg == WIM_DATA) {
        WAVEHDR* pWaveHdr = (WAVEHDR*)dwParam1;
        std::vector<short> buffer((short*)pWaveHdr->lpData, (short*)pWaveHdr->lpData + pWaveHdr->dwBytesRecorded / sizeof(short));
        if (!paused) {
            std::vector<int> audioSamples(buffer.begin(), buffer.end());

            double rms = calculateRMS(audioSamples);

            double decibels = rmsToDecibels(rms);

            std::cout << "\rCurrent Sound Level: " << decibels << " dB            " << std::flush;
        }

        waveInAddBuffer(hwi, pWaveHdr, sizeof(WAVEHDR));
    }
}

int main() {
    HWAVEIN hWaveIn;
    WAVEFORMATEX waveFormat;
    WAVEHDR waveHeader;
    std::vector<short> buffer(BUFFER_SIZE);
    MMRESULT result;

    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = NUM_CHANNELS;
    waveFormat.nSamplesPerSec = SAMPLE_RATE;
    waveFormat.wBitsPerSample = BITS_PER_SAMPLE;
    waveFormat.nBlockAlign = (NUM_CHANNELS * BITS_PER_SAMPLE) / 8;
    waveFormat.nAvgBytesPerSec = SAMPLE_RATE * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    result = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open audio input device." << std::endl;
        return 1;
    }

    waveHeader.lpData = reinterpret_cast<LPSTR>(buffer.data());
    waveHeader.dwBufferLength = BUFFER_SIZE * sizeof(short);
    waveHeader.dwBytesRecorded = 0;
    waveHeader.dwFlags = 0;
    waveHeader.dwLoops = 0;

    result = waveInPrepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to prepare header." << std::endl;
        return 1;
    }

    result = waveInAddBuffer(hWaveIn, &waveHeader, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to add buffer." << std::endl;
        return 1;
    }

    result = waveInStart(hWaveIn);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to start recording." << std::endl;
        return 1;
    }

    std::thread inputThread(handleInput);

    inputThread.join();

    result = waveInStop(hWaveIn);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to stop recording." << std::endl;
    }

    result = waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to unprepare header." << std::endl;
    }

    result = waveInClose(hWaveIn);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to close audio input device." << std::endl;
    }

    return 0;
}