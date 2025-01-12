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
void encode_audio_lossy(int16_t* buffer, int frames,int channels, int M, const string& output_file, int predictor_order, int num_bits) {
    Golomb encoder(M, false, output_file);

    // Encode first samples (no prediction for these)
    for (int i = 0; i < predictor_order; i++) {
        if(channels == 2){
            // quantized the samples to have the "num_bits" data rate
            int16_t left_sample = quantize_sample(buffer[i * 2],num_bits);
            int16_t right_sample = quantize_sample(buffer[i * 2 + 1],num_bits);

            encoder.encode(left_sample);     // Left channel
            encoder.encode(right_sample); // Right channel
                
            // save the samples after reversing the quantization in the buffer, in order to have the same samples in the encoder and decoder
            buffer[i*2] = dequantize_sample(left_sample,num_bits);
            buffer[i*2+1] = dequantize_sample(right_sample,num_bits);
        }
        else{
            int16_t sample = quantize_sample(buffer[i],num_bits);
            encoder.encode(sample);
            buffer[i] = dequantize_sample(sample,num_bits);
        }
    }

    
    // Encode residuals with prediction
    for (int i = predictor_order; i < frames; i++) {
        if(channels == 2){
            int left_idx = i * 2;
            int right_idx = i * 2 + 1;

            int16_t prediction_left = predict_sample(buffer, channels, left_idx, predictor_order);
            int16_t residual_left = buffer[left_idx] - prediction_left;
            residual_left = quantize_sample(residual_left,num_bits);
            encoder.encode(residual_left);
            buffer[left_idx] = prediction_left+dequantize_sample( residual_left,num_bits);      // alter the buffer position so we dont have the error propagation
            
            int16_t prediction_right = predict_sample(buffer, channels, right_idx, predictor_order);
            int16_t residual_right = buffer[right_idx] - prediction_right;
            residual_right = quantize_sample(residual_right,num_bits);
            buffer[right_idx] = prediction_right+dequantize_sample(residual_right,num_bits);      // alter the buffer position so we dont have the error propagation
            encoder.encode(residual_right);
        }
        else{
            int16_t prediction = predict_sample(buffer, channels, i, predictor_order);
            int16_t residual = buffer[i] - prediction;
            residual = quantize_sample(residual,num_bits);
            buffer[i] = prediction + dequantize_sample(residual,num_bits);      // alter the buffer position so we dont have the error propagation
            encoder.encode(residual);
        }
    }

    encoder.end();
}

vector<int16_t> decode_audio_lossy(int M, const string& input_file, int frames, int channels, int predictor_order,int num_bits) {
    Golomb decoder(M, true, input_file);
    vector<int> residuals = decoder.decode();
    decoder.end();

    vector<int16_t> decoded;
    // Decode first samples directly
    for (int i = 0; i < predictor_order; i++) {
        if(channels == 2){
            decoded.push_back(dequantize_sample(residuals[i * 2],num_bits));     // Left channel
            decoded.push_back(dequantize_sample(residuals[i * 2 + 1],num_bits) ); // Right channel
        }
        else{
            decoded.push_back(dequantize_sample(residuals[i],num_bits));     // Left channel
        }
    }

    // Reconstruct signal using residuals and predictions
    for (int i = predictor_order; i < frames; i++) {
        if(channels == 2) {
            int left_idx = i * 2;
            int right_idx = i * 2 + 1;
            
            int16_t prediction_left = predict_sample(decoded.data(), channels, left_idx, predictor_order);
            int16_t left_sample = prediction_left + dequantize_sample(residuals[left_idx],num_bits);
            decoded.push_back(left_sample);
            
            int16_t prediction_right = predict_sample(decoded.data(), channels, right_idx, predictor_order);
            int16_t right_sample = prediction_right + dequantize_sample(residuals[right_idx],num_bits);
            decoded.push_back(right_sample);
        }
        else{
            int16_t prediction = predict_sample(decoded.data(), channels, i, predictor_order);
            int16_t sample = prediction + dequantize_sample(residuals[i],num_bits);
            decoded.push_back(sample);
        }
        
    }

    return decoded;
}

// Helper for lossy encoding
void perform_lossy_encoding(int16_t *buffer, int frames, int M, int predictor_order, int num_bits, int sample_rate, int channels) {
    // make a copy of the buffer with the original data so we don't alter the original one
    int16_t *buffercpy = new int16_t[frames * channels]; 
    memcpy(buffercpy,buffer,frames*channels*sizeof(int16_t));       

    // auto start = high_resolution_clock::now();
    encode_audio_lossy(buffercpy, frames, channels, M, "error_lossy.bin", predictor_order, num_bits);
    // auto end = high_resolution_clock::now();
    // cout << "Lossy encoding completed in " << duration_cast<milliseconds>(end - start).count() << " ms\n";

    // start = high_resolution_clock::now();
    auto decoded = decode_audio_lossy(M, "error_lossy.bin", frames, channels, predictor_order, num_bits);
    // end = high_resolution_clock::now();
    // cout << "Lossy decoding completed in " << duration_cast<milliseconds>(end - start).count() << " ms\n";

    save_wav("reconstructed_lossy.wav", decoded, sample_rate, channels);
    cout << "Successfully generated 'reconstructed_lossy.wav' using lossy predictive coding!" << endl;
    vector<int16_t> original_audio(buffer,buffer+frames*channels); 
    double snr = calculate_snr(original_audio,decoded);
    cout << "Signal-to-Noise Ratio (SNR): " << snr << " dB" << endl;

    free(buffercpy);
}
