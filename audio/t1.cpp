#include <iostream>
#include <sndfile.h>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

int main (int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Please provide a sound file as an argument." << endl;
        return 1;
    }

    const char *filename = argv[1];
    SF_INFO sfinfo;

    SNDFILE *sf = sf_open(filename, SFM_READ, &sfinfo);
    if (!sf) {
        cout << "Error opening the file" << endl;
        return 1;
    }

    int sample_rate = sfinfo.samplerate;
    int channels = sfinfo.channels;
    int frames = sfinfo.frames;
    float duration = static_cast<float>(frames) / sample_rate;

    cout << "Sample rate: " << sample_rate << endl;
    cout << "Channels: " << channels << endl;
    cout << "Frames: " << frames << endl;
    cout << "Duration: " << duration << " seconds" << endl;

    if (channels != 2) {
        cout << "This example is designed for stereo (2 channels) audio files." << endl;
        sf_close(sf);
        return 1;
    }

    int numSamples = frames * channels;
    float* buffer = new float[numSamples];

    sf_readf_float(sf, buffer, frames);

    // Separate left and right channels
    for (int i = 0; i < frames; i++) {
        float left_sample = buffer[i * 2];     // Left channel
        float right_sample = buffer[i * 2 + 1]; // Right channel

        cout << "Frame " << i << " - Left: " << left_sample << ", Right: " << right_sample << endl;
    }

    sf_close(sf);
    delete[] buffer;

    return 0;
}
