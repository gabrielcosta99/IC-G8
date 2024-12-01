#include <iostream>
#include <sndfile.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <cmath>
#include "../Golomb.h"

#define DEFAULT_M_VALUE 32768

using namespace std;
namespace fs = filesystem;
using namespace chrono;

// Function Prototypes
void save_wav(const char* output_filename, const vector<int>& data, int sample_rate, int channels);
int calculate_dynamic_m(const int* buffer, int frames);
void encode_audio(const int* buffer, int frames, int M, const string& output_file);
vector<int> decode_audio(int M, const string& input_file, int frames, bool inter_channel = false);
vector<int> interchannel_encode(const int* buffer, int frames, int M, const string& output_file);
vector<int> interchannel_decode(int M, const string& input_file, int frames);

// Save WAV file
void save_wav(const char* output_filename, const vector<int>& data, int sample_rate, int channels) {
    SF_INFO sfinfo = { 0 };
    sfinfo.samplerate = sample_rate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sf_out = sf_open(output_filename, SFM_WRITE, &sfinfo);
    if (!sf_out) {
        cerr << "Error: Could not open output WAV file." << endl;
        return;
    }

    sf_writef_int(sf_out, data.data(), data.size() / channels);
    sf_close(sf_out);
}

// Calculate dynamic M
int calculate_dynamic_m(const int* buffer, int frames) {
    vector<int> prediction_errors;
    int previous_sample = buffer[0];

    for (int i = 1; i < frames; ++i) {
        int current_sample = buffer[i];
        int error = current_sample - previous_sample;
        prediction_errors.push_back(abs(error));
        previous_sample = current_sample;
    }

    double mean_error = 0;
    for (int error : prediction_errors) {
        mean_error += error;
    }
    mean_error /= prediction_errors.size();

    return (int)pow(2, ceil(log2(mean_error)));
}

// Encode audio
void encode_audio(const int* buffer, int frames, int M, const string& output_file) {
    Golomb encoder(M, false, output_file);

    int previous_left = buffer[0];
    int previous_right = buffer[1];

    encoder.encode_val(previous_left);
    encoder.encode_val(previous_right);

    for (int i = 1; i < frames; ++i) {
        int left_sample = buffer[i * 2];
        int right_sample = buffer[i * 2 + 1];

        encoder.encode_val(left_sample - previous_left);
        encoder.encode_val(right_sample - previous_right);

        previous_left = left_sample;
        previous_right = right_sample;
    }
    encoder.end();
}

// Decode audio
vector<int> decode_audio(int M, const string& input_file, int frames, bool inter_channel) {
    Golomb decoder(M, true, input_file);
    vector<int> errors = decoder.decode();
    decoder.end();

    vector<int> decoded;
    int previous_left_sample = errors[0];
    int previous_right_sample = errors[1];

    decoded.push_back(previous_left_sample);
    decoded.push_back(previous_right_sample);

    for (size_t i = 2; i < errors.size(); i += 2) {
        int left_sample = previous_left_sample + errors[i];
        int right_sample = previous_right_sample + errors[i + 1];

        decoded.push_back(left_sample);
        decoded.push_back(right_sample);

        previous_left_sample = left_sample;
        previous_right_sample = right_sample;
    }

    return decoded;
}

// Interchannel encode
vector<int> interchannel_encode(const int* buffer, int frames, int M, const string& output_file) {
    Golomb encoder(M, false, output_file);

    for (int i = 0; i < frames; ++i) {
        int left_sample = buffer[i * 2];
        int right_sample = buffer[i * 2 + 1];

        encoder.encode_val(left_sample);
        encoder.encode_val(right_sample - left_sample);
    }
    encoder.end();

    return {};
}

// Interchannel decode
vector<int> interchannel_decode(int M, const string& input_file, int frames) {
    Golomb decoder(M, true, input_file);
    vector<int> values = decoder.decode();
    decoder.end();

    vector<int> decoded;
    int left_sample = values[0];
    int right_sample = left_sample + values[1];

    decoded.push_back(left_sample);
    decoded.push_back(right_sample);

    for (size_t i = 1; i < values.size(); i += 2) {
        left_sample = values[i];
        right_sample = left_sample + values[i + 1];

        decoded.push_back(left_sample);
        decoded.push_back(right_sample);
    }

    return decoded;
}

// Main Function
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: ./program <audio_file> [M]" << endl;
        return 1;
    }

    const char* filename = argv[1];
    SF_INFO sfinfo = { 0 };
    SNDFILE* sf = sf_open(filename, SFM_READ, &sfinfo);

    if (!sf) {
        cerr << "Error opening file: " << filename << endl;
        return 1;
    }

    int sample_rate = sfinfo.samplerate;
    int channels = sfinfo.channels;
    int frames = sfinfo.frames;

    if (channels != 2) {
        cerr << "This program only supports stereo audio files." << endl;
        sf_close(sf);
        return 1;
    }

    int* buffer = new int[frames * channels];
    sf_readf_int(sf, buffer, frames);

    // Calculate or read M
    int M = (argc < 3) ? calculate_dynamic_m(buffer, frames) : atoi(argv[2]);
    cout << "Using M: " << M << endl;

    auto start = high_resolution_clock::now();

    // Encoding and decoding
    encode_audio(buffer, frames, M, "error.bin");
    vector<int> decoded = decode_audio(M, "error.bin", frames);

    save_wav("reconstructed.wav", decoded, sample_rate, channels);

    // Interchannel coding
    interchannel_encode(buffer, frames, M, "inter_error.bin");
    vector<int> inter_decoded = interchannel_decode(M, "inter_error.bin", frames);

    save_wav("reconstructed_inter.wav", inter_decoded, sample_rate, channels);

    auto end = high_resolution_clock::now();
    cout << "Execution time: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    sf_close(sf);
    delete[] buffer;

    return 0;
}
