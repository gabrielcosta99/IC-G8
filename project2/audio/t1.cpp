#include <iostream>
#include <sndfile.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <cmath>
#include "../include/Golomb.h"

#define DEFAULT_M_VALUE 32768

using namespace std;
namespace fs = filesystem;
using namespace chrono;


// Save WAV file
void save_wav(const char* output_filename, const vector<int16_t>& data, int sample_rate, int channels) {
    SF_INFO sfinfo = { 0 };
    sfinfo.samplerate = sample_rate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sf_out = sf_open(output_filename, SFM_WRITE, &sfinfo);
    if (!sf_out) {
        cerr << "Error: Could not open output WAV file." << endl;
        return;
    }

    sf_writef_short(sf_out, data.data(), data.size() / channels);
    sf_close(sf_out);
}

// Calculate dynamic M
int calculate_dynamic_m(const int16_t* buffer, int frames) {
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

int predict_sample(const int16_t* buffer, int n, int order, bool is_left_channel) {
    if (order == 0) return 0;

    // Adjust indexing for stereo
    int step = 2; // Each frame has two samples (left and right)
    int offset = is_left_channel ? 0 : 1;

    if (order == 1) return buffer[n - step + offset];
    if (order == 2) return 2 * buffer[n - step + offset] - buffer[n - 2 * step + offset];
    if (order == 3) return 3 * buffer[n - step + offset] - 3 * buffer[n - 2 * step + offset] + buffer[n - 3 * step + offset];
    if (order == 4) return buffer[n - step + offset] + (buffer[n - step + offset] - buffer[n - 2*step + offset]);
    return 0; // Default fallback
}
// Encode audio
void encode_audio(const int16_t* buffer, int frames, int M, const string& output_file, int predictor_order) {
    Golomb encoder(M, false, output_file);

    // Encode first samples (no prediction for these)
    for (int i = 0; i < predictor_order; ++i) {
        encoder.encode(buffer[i * 2]);     // Left channel
        encoder.encode(buffer[i * 2 + 1]); // Right channel
    }

    // Encode residuals with prediction
    for (int i = predictor_order; i < frames; ++i) {
        int left_idx = i * 2;
        int right_idx = i * 2 + 1;

        int16_t prediction_left = predict_sample(buffer, left_idx, predictor_order, true);
        int16_t residual_left = buffer[left_idx] - prediction_left;
        encoder.encode(residual_left);

        int16_t prediction_right = predict_sample(buffer, right_idx, predictor_order, false);
        int16_t residual_right = buffer[right_idx] - prediction_right;
        encoder.encode(residual_right);

        // printf("residual_right: %d, diff_right: %d\n",residual_right,buffer[right_idx]-buffer[right_idx-2]);
    }

    encoder.end();
}

vector<int16_t> decode_audio(int M, const string& input_file, int frames, int predictor_order) {
    Golomb decoder(M, true, input_file);
    vector<int> residuals = decoder.decode();
    decoder.end();

    vector<int16_t> decoded;

    // Decode first samples directly
    for (int i = 0; i < predictor_order; ++i) {
        decoded.push_back(residuals[i * 2]);     // Left channel
        decoded.push_back(residuals[i * 2 + 1]); // Right channel
    }

    // Reconstruct signal using residuals and predictions
    for (int i = predictor_order; i < frames; ++i) {
        int left_idx = i * 2;
        int right_idx = i * 2 + 1;

        int16_t prediction_left = predict_sample(decoded.data(), left_idx, predictor_order, true);
        decoded.push_back(residuals[left_idx] + prediction_left);

        int16_t prediction_right = predict_sample(decoded.data(), right_idx, predictor_order, false);
        decoded.push_back(residuals[right_idx] + prediction_right);
    }

    return decoded;
}



// Interchannel encode
void interchannel_encode(const int16_t* buffer, int frames, int M, const string& output_file) {
    Golomb encoder(M, false, output_file);

    for (int i = 0; i < frames; ++i) {
        int left_sample = buffer[i * 2];
        int right_sample = buffer[i * 2 + 1];

        encoder.encode(left_sample);
        encoder.encode(right_sample - left_sample);
    }
    encoder.end();

    // return {};
}

// Interchannel decode
vector<int16_t> interchannel_decode(int M, const string& input_file, int frames) {
    Golomb decoder(M, true, input_file);
    vector<int> values = decoder.decode();
    decoder.end();

    vector<int16_t> decoded;
    int16_t left_sample;
    int16_t right_sample;

    for (size_t i = 0; i < values.size(); i += 2) {
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
        cout << "Usage: ./program <audio_file>" << endl;
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

    cout << "Sample rate: " << sample_rate << endl;
    cout << "Channels: " << channels << endl;
    cout << "Frames: " << frames << endl;


    int16_t* buffer = new int16_t[frames * channels];
    sf_readf_short(sf, buffer, frames);

    string input;
    // ask if the user wants to give an M
    printf("Would you like to give an M? [y/N] ");
    input = cin.get();
    int M;
    if(input=="y"){
        cout << endl;
        cout << "Type a number (preferebly higher than 100000): ";

        cin >> M;
        M = (int)pow(2, ceil(log2(M)));
    }
    else{
        M = calculate_dynamic_m(buffer, frames);
    }
    // Calculate or read M
    cout << "Using M: " << M << endl;

    cout << "Would you like to try:"<<endl;
    cout << "1. Lossless encoding" << endl;
    cout << "2. Lossy encoding" << endl;
    cin >> input;
    if(input == "1"){
        cout << "Doing lossless encoding..." << endl;
    // Encoding and decoding
        auto start = high_resolution_clock::now();
        int predictor_order = 3; 
        encode_audio(buffer, frames, M, "error.bin", predictor_order);
        vector<int16_t> decoded = decode_audio(M, "error.bin", frames, predictor_order);


        save_wav("reconstructed.wav", decoded, sample_rate, channels);

        // Interchannel coding
        interchannel_encode(buffer, frames, M, "inter_error.bin");
        vector<int16_t> inter_decoded = interchannel_decode(M, "inter_error.bin", frames);

        save_wav("reconstructed_inter.wav", inter_decoded, sample_rate, channels);

        auto end = high_resolution_clock::now();
        cout << "Execution time: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;


    } else if(input == "2"){
        cout << "Doing lossy encoding..." << endl;
        auto start = high_resolution_clock::now();
        
        
        auto end = high_resolution_clock::now();
        cout << "Execution time: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    }
        sf_close(sf);
    delete[] buffer;

    return 0;
}
