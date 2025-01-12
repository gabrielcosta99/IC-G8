#include <iostream>
#include <sndfile.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <cmath>
#include <cstring>
#include "../include/Golomb.h"
#include "./main.h"

#define DEFAULT_M_VALUE 32768

using namespace std;
namespace fs = filesystem;
using namespace chrono;


// Encode audio
void encode_audio(const int16_t* buffer, int frames, int channels, int M, const string& output_file, int predictor_order) {
    Golomb encoder(M, false, output_file);
    // Encode first 'n' samples (no prediction for these)
    for (int i = 0; i < predictor_order; i++) {
        if(channels == 2){
            encoder.encode(buffer[i * 2]);     // Left channel
            encoder.encode(buffer[i * 2 + 1]); // Right channel
        }
        else{
            encoder.encode(buffer[i]);
        }
    }

    // Encode residuals with prediction
    for (int i = predictor_order; i < frames; i++) {
        if(channels == 2){
            int left_idx = i * 2;
            int right_idx = i * 2 + 1;

            int16_t prediction_left = predict_sample(buffer, channels, left_idx, predictor_order);
            int16_t residual_left = buffer[left_idx] - prediction_left;
            encoder.encode(residual_left);

            int16_t prediction_right = predict_sample(buffer, channels, right_idx, predictor_order);
            int16_t residual_right = buffer[right_idx] - prediction_right;
            encoder.encode(residual_right);

            // printf("residual_right: %d, diff_right: %d\n",residual_right,buffer[right_idx]-buffer[right_idx-2]);
        }
        else{
            // cout << channels << endl;
            int16_t prediction = predict_sample(buffer, channels, i, predictor_order);
            int16_t residual = buffer[i] - prediction;
            encoder.encode(residual);
        }
    }
    encoder.end();
}

vector<int16_t> decode_audio(int M, const string& input_file, int frames,int channels, int predictor_order) {
    Golomb decoder(M, true, input_file);
    vector<int> residuals = decoder.decode();
    decoder.end();

    vector<int16_t> decoded;

    // Decode first 'n' samples directly
    for (int i = 0; i < predictor_order; i++) {
        if(channels == 2){
            decoded.push_back(residuals[i * 2]);     // Left channel
            decoded.push_back(residuals[i * 2 + 1]); // Right channel
        }
        else{
            decoded.push_back(residuals[i]);
        }
      
    }

    // Reconstruct signal using residuals and predictions
    for (int i = predictor_order; i < frames; i++) {
        if(channels == 2){
            int left_idx = i * 2;
            int right_idx = i * 2 + 1;

            int16_t prediction_left = predict_sample(decoded.data(), channels, left_idx, predictor_order);
            decoded.push_back(residuals[left_idx] + prediction_left);

            int16_t prediction_right = predict_sample(decoded.data(), channels, right_idx, predictor_order);
            decoded.push_back(residuals[right_idx] + prediction_right);
        }
        else{
            int16_t prediction = predict_sample(decoded.data(), channels, i, predictor_order);
            decoded.push_back(residuals[i] + prediction);
        }
        
    }
    return decoded;
}



// Interchannel encode
void interchannel_encode(const int16_t* buffer, int frames, int M, const string& output_file) {
    Golomb encoder(M, false, output_file);

    for (int i = 0; i < frames; i++) {
        int16_t left_sample = buffer[i * 2];
        int16_t right_sample = buffer[i * 2 + 1];

        encoder.encode(left_sample);
        encoder.encode(right_sample - left_sample);
    }
    encoder.end();

}

// Interchannel decode
vector<int16_t> interchannel_decode(int M, const string& input_file, int frames) {
    Golomb decoder(M, true, input_file);
    vector<int> values = decoder.decode();
    decoder.end();

    vector<int16_t> decoded;

    for (size_t i = 0; i < values.size()-1; i += 2) {
        int16_t left_sample = values[i];
        int16_t right_sample = left_sample + values[i + 1];

        decoded.push_back(left_sample);
        decoded.push_back(right_sample);
    }

    return decoded;
}


// Helper for lossless encoding
void perform_lossless_encoding(int16_t *buffer, int frames, int M, int predictor_order, int sample_rate, int channels) {
    // auto start = high_resolution_clock::now();
    encode_audio(buffer, frames, channels, M, "error.bin", predictor_order);
    // auto end = high_resolution_clock::now();
    // cout << "Lossless encoding completed in " << duration_cast<milliseconds>(end - start).count() << " ms\n";

    // start = high_resolution_clock::now();
    auto decoded = decode_audio(M, "error.bin", frames, channels, predictor_order);
    // end = high_resolution_clock::now();
    // cout << "Lossless decoding completed in " << duration_cast<milliseconds>(end - start).count() << " ms\n";

    save_wav("reconstructed.wav", decoded, sample_rate, channels);
    cout << "Successfully generated 'reconstructed.wav' using lossless predictive coding!" << endl;

    // vector<int16_t> original_audio(buffer,buffer+frames*channels); 
    // double snr = calculate_snr(original_audio,decoded);
    // cout << "Predictive Signal-to-Noise Ratio (SNR): " << snr << " dB" << endl;

    if(channels == 2){
        // Interchannel coding
        // start = high_resolution_clock::now();
        interchannel_encode(buffer, frames, M, "inter_error.bin");
        // end = high_resolution_clock::now();
        // cout << "Interchannel encoding took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

        // start = high_resolution_clock::now();
        vector<int16_t> inter_decoded = interchannel_decode(M, "inter_error.bin", frames);
        // end = high_resolution_clock::now();
        // cout << "Interchannel decoding took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

        save_wav("reconstructed_inter.wav", inter_decoded, sample_rate, channels);
        cout << "Successfully generated 'reconstructed_inter.wav' using lossless inter-channel coding!" << endl;
        
        // snr = calculate_snr(original_audio,inter_decoded);
        // cout << "Interchannel Signal-to-Noise Ratio (SNR): " << snr << " dB" << endl;
    }
}

