#include <iostream>
#include <sndfile.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <cmath>
#include <cstring>
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




double calculate_snr(const std::vector<int16_t>& original, const std::vector<int16_t>& reconstructed) {
    if (original.size() != reconstructed.size()) {
        std::cerr << "Error: Original and reconstructed signals must have the same size." << std::endl;
        return -1; // Return an error value
    }

    double signal_power = 0.0;
    double noise_power = 0.0;

    for (size_t i = 0; i < original.size(); ++i) {
        signal_power += original[i] * original[i];
        double noise = original[i] - reconstructed[i];
        noise_power += noise * noise;
    }

    if (noise_power == 0) {
        std::cerr << "Warning: Noise power is zero. SNR is infinite." << std::endl;
        return std::numeric_limits<double>::infinity();
    }

    // signal_power /= original.size();
    // noise_power /= original.size();

    return 10 * log10(signal_power / noise_power);
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

int16_t quantize_sample(int16_t sample, int num_bits) {

    int16_t diff = 16-num_bits;
    int16_t quantized_sample = sample >> diff ;

    return quantized_sample;
}

int16_t dequantize_sample(int16_t sample, int num_bits) {

    int16_t diff = 16-num_bits;
    int16_t dequantized_sample = sample << diff;

    return dequantized_sample;
}

// Calculate dynamic M
int calculate_dynamic_m(int16_t* buffer, int frames,int channels, bool isLossless,int predictor_order,int num_bits) {
    vector<int16_t> prediction_errors;

    if(isLossless){
        for (int i = 0; i < predictor_order; i++) {
            prediction_errors.push_back(buffer[i * 2]);     // Left channel
            prediction_errors.push_back(buffer[i * 2 + 1]); // Right channel
        }
        for (int i = predictor_order; i < frames; i++) {
            int left_idx = i * 2;
            int right_idx = i * 2 + 1;

            int16_t prediction_left = predict_sample(buffer, left_idx, predictor_order, true);
            int16_t residual_left = buffer[left_idx] - prediction_left;
            prediction_errors.push_back(abs(residual_left));

            int16_t prediction_right = predict_sample(buffer, right_idx, predictor_order, false);
            int16_t residual_right = buffer[right_idx] - prediction_right;
            prediction_errors.push_back(abs(residual_right));
        }
    }
    else{
        int16_t *buffercpy = new int16_t[frames * channels]; 
        memcpy(buffercpy,buffer,frames*channels*sizeof(int16_t));
        for (int i = 0; i < predictor_order; i++) {
            int16_t left_sample = quantize_sample(buffer[i * 2],num_bits);
            int16_t right_sample = quantize_sample(buffer[i * 2 + 1],num_bits);
            prediction_errors.push_back(left_sample);     // Left channel
            prediction_errors.push_back(right_sample); // Right channel
            buffercpy[i*2] = dequantize_sample(left_sample,num_bits);
            buffercpy[i*2+1] = dequantize_sample(right_sample,num_bits);
        }

        
        // Encode residuals with prediction
        for (int i = predictor_order; i < frames; i++) {
            int left_idx = i * 2;
            int right_idx = i * 2 + 1;

            int16_t prediction_left = predict_sample(buffercpy, left_idx, predictor_order, true);
            int16_t residual_left = buffercpy[left_idx] - prediction_left;
            residual_left = quantize_sample(residual_left,num_bits);
            prediction_errors.push_back(abs(residual_left));
            buffercpy[left_idx] = prediction_left+dequantize_sample( residual_left,num_bits);      // alter the buffercpy position so we dont have the error propagation
            
            int16_t prediction_right = predict_sample(buffercpy, right_idx, predictor_order, false);
            int16_t residual_right = buffercpy[right_idx] - prediction_right;
            residual_right = quantize_sample(residual_right,num_bits);
            buffercpy[right_idx] = prediction_right+dequantize_sample(residual_right,num_bits);      // alter the buffercpy position so we dont have the error propagation
            prediction_errors.push_back(abs(residual_right));

        }
        free(buffercpy);
    }

    double mean_error = 0;
    for (int16_t error : prediction_errors) {
        // cout << error << endl;
        mean_error += error;
    }
    mean_error /= prediction_errors.size();

    int dynamic_m = (int)pow(2, ceil(log2(mean_error)));
    dynamic_m = max(dynamic_m, 2); // Ensure M is at least 2
    return dynamic_m;
}

// Encode audio
void encode_audio(const int16_t* buffer, int frames, int M, const string& output_file, int predictor_order) {
    Golomb encoder(M, false, output_file);

    // Encode first samples (no prediction for these)
    for (int i = 0; i < predictor_order; i++) {
        encoder.encode(buffer[i * 2]);     // Left channel
        encoder.encode(buffer[i * 2 + 1]); // Right channel
    }

    // Encode residuals with prediction
    for (int i = predictor_order; i < frames; i++) {
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
    for (int i = 0; i < predictor_order; i++) {
        decoded.push_back(residuals[i * 2]);     // Left channel
        decoded.push_back(residuals[i * 2 + 1]); // Right channel
    }

    // Reconstruct signal using residuals and predictions
    for (int i = predictor_order; i < frames; i++) {
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

    for (int i = 0; i < frames; i++) {
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

    for (size_t i = 0; i < values.size()-1; i += 2) {
        left_sample = values[i];
        right_sample = left_sample + values[i + 1];

        decoded.push_back(left_sample);
        decoded.push_back(right_sample);
    }

    return decoded;
}


// Encode audio
void encode_audio_lossy(int16_t* buffer, int frames, int M, const string& output_file, int predictor_order, int num_bits) {
    Golomb encoder(M, false, output_file);

    // ofstream outfile("encode.txt");
    // outfile << "ENCODE"<<endl;

    // Encode first samples (no prediction for these)
    for (int i = 0; i < predictor_order; i++) {
        int16_t left_sample = quantize_sample(buffer[i * 2],num_bits);
        int16_t right_sample = quantize_sample(buffer[i * 2 + 1],num_bits);
        encoder.encode(left_sample);     // Left channel
        encoder.encode(right_sample); // Right channel
        
        // outfile << "left (original | after quant and dequant): " << buffer[i*2] 
        // << " | " << dequantize_sample(left_sample,num_bits) << endl;
        buffer[i*2] = dequantize_sample(left_sample,num_bits);
        
        // outfile << "right (original | after quant and dequant): " << buffer[i*2+1] 
        // << " | " << dequantize_sample(right_sample,num_bits) << endl;
        buffer[i*2+1] = dequantize_sample(right_sample,num_bits);
    }

    
    // Encode residuals with prediction
    for (int i = predictor_order; i < frames; i++) {
        int left_idx = i * 2;
        int right_idx = i * 2 + 1;

        int16_t prediction_left = predict_sample(buffer, left_idx, predictor_order, true);
        int16_t residual_left = buffer[left_idx] - prediction_left;
        // printf("residual_left: %d\n",residual_left);
        residual_left = quantize_sample(residual_left,num_bits);

        // outfile << "left (original | after quant and dequant): " << buffer[left_idx] 
        // << " | " << prediction_left + dequantize_sample(residual_left, num_bits) << endl;
        encoder.encode(residual_left);
        // printf("residual_left_quantized: %d\n",residual_left);
        buffer[left_idx] = prediction_left+dequantize_sample( residual_left,num_bits);      // alter the buffer position so we dont have the error propagation
        
        int16_t prediction_right = predict_sample(buffer, right_idx, predictor_order, false);
        int16_t residual_right = buffer[right_idx] - prediction_right;
        residual_right = quantize_sample(residual_right,num_bits);

        // outfile << "right (original | after quant and dequant): " << buffer[right_idx] 
        // << " | " << prediction_right + dequantize_sample(residual_right, num_bits) << endl;
        buffer[right_idx] = prediction_right+dequantize_sample(residual_right,num_bits);      // alter the buffer position so we dont have the error propagation
        encoder.encode(residual_right);

        // printf("residual_right: %d, diff_right: %d\n",residual_right,buffer[right_idx]-buffer[right_idx-2]);
    }

    encoder.end();
}

vector<int16_t> decode_audio_lossy(int M, const string& input_file, int frames, int predictor_order,int num_bits) {
    Golomb decoder(M, true, input_file);
    vector<int> residuals = decoder.decode();
    decoder.end();

    // vector<int> residuals_dequantized;

    // for(int i= 0; i<frames;i++){
    //     residuals_dequantized.push_back(dequantize_sample(residuals[i*2],num_bits));
    //     residuals_dequantized.push_back(dequantize_sample(residuals[i*2+1],num_bits));
    // }

    vector<int16_t> decoded;
    // Decode first samples directly
    for (int i = 0; i < predictor_order; i++) {
        decoded.push_back(dequantize_sample(residuals[i * 2],num_bits));     // Left channel
        decoded.push_back(dequantize_sample(residuals[i * 2 + 1],num_bits) ); // Right channel
    }
    // ofstream outfile("decode.txt");
    // outfile << "DECODE"<<endl;
    // Reconstruct signal using residuals and predictions
    for (int i = predictor_order; i < frames; i++) {
        int left_idx = i * 2;
        int right_idx = i * 2 + 1;
        
        int16_t prediction_left = predict_sample(decoded.data(), left_idx, predictor_order, true);
        int16_t left_sample = prediction_left + dequantize_sample(residuals[left_idx],num_bits);
        decoded.push_back( left_sample);
        // outfile << "left: " <<left_sample << " | " << buffer[left_idx] << endl;
        
        int16_t prediction_right = predict_sample(decoded.data(), right_idx, predictor_order, false);
        int16_t right_sample = prediction_right + dequantize_sample(residuals[right_idx],num_bits);
        decoded.push_back(right_sample );
        // outfile << "right: " <<right_sample << " | " << buffer[right_idx] << endl;
    }

    return decoded;
}

// Helper for lossless encoding
void perform_lossless_encoding(int16_t *buffer, int frames, int M, int predictor_order, int sample_rate, int channels) {
    auto start = high_resolution_clock::now();
    encode_audio(buffer, frames, M, "error.bin", predictor_order);
    auto end = high_resolution_clock::now();
    cout << "Lossless encoding completed in " << duration_cast<milliseconds>(end - start).count() << " ms\n";

    start = high_resolution_clock::now();
    auto decoded = decode_audio(M, "error.bin", frames, predictor_order);
    end = high_resolution_clock::now();
    cout << "Lossless decoding completed in " << duration_cast<milliseconds>(end - start).count() << " ms\n";

    save_wav("reconstructed.wav", decoded, sample_rate, channels);

    // Interchannel coding
    start = high_resolution_clock::now();
    interchannel_encode(buffer, frames, M, "inter_error.bin");
    end = high_resolution_clock::now();
    cout << "Interchannel encoding took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    start = high_resolution_clock::now();
    vector<int16_t> inter_decoded = interchannel_decode(M, "inter_error.bin", frames);
    end = high_resolution_clock::now();
    cout << "Interchannel decoding took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    save_wav("reconstructed_inter.wav", inter_decoded, sample_rate, channels);
    vector<int16_t> original_audio(buffer,buffer+frames*channels); 
    double snr = calculate_snr(original_audio,decoded);
    cout << "Predictive Signal-to-Noise Ratio (SNR): " << snr << " dB" << endl;
    
    snr = calculate_snr(original_audio,inter_decoded);
    cout << "Interchannel Signal-to-Noise Ratio (SNR): " << snr << " dB" << endl;

}

// Helper for lossy encoding
void perform_lossy_encoding(int16_t *buffer, int frames, int M, int predictor_order, int num_bits, int sample_rate, int channels) {
    int16_t *buffercpy = new int16_t[frames * channels]; 
    memcpy(buffercpy,buffer,frames*channels*sizeof(int16_t));

    auto start = high_resolution_clock::now();
    encode_audio_lossy(buffercpy, frames, M, "error_lossy.bin", predictor_order, num_bits);
    auto end = high_resolution_clock::now();
    cout << "Lossy encoding completed in " << duration_cast<milliseconds>(end - start).count() << " ms\n";

    start = high_resolution_clock::now();
    auto decoded = decode_audio_lossy(M, "error_lossy.bin", frames, predictor_order, num_bits);
    end = high_resolution_clock::now();
    cout << "Lossy decoding completed in " << duration_cast<milliseconds>(end - start).count() << " ms\n";

    save_wav("reconstructed_lossy.wav", decoded, sample_rate, channels);

    vector<int16_t> original_audio(buffer,buffer+frames*channels); 
    double snr = calculate_snr(original_audio,decoded);
    cout << "Signal-to-Noise Ratio (SNR): " << snr << " dB" << endl;

    free(buffercpy);
}

// Main Function
int t2(const char* filename) {
    

    // const char* filename = argv[1];
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
    printf("Would you like to give an M value? [y/N] ");
    input = cin.get();
    int M;
    bool dynamicM = false;
    if (input == "y" || input == "Y") {
        cout << "Enter a number: ";
        cin >> M;
        M = (int)pow(2, ceil(log2(M))); // Round to nearest power of 2
    } else {
        dynamicM = true;
    }
    

    cout << "Choose encoding mode:\n"
         << "1. Lossless encoding\n"
         << "2. Lossy encoding\n"
         << "Enter your choice: ";
    cin >> input;

    bool isLossless = (input == "1");
    int predictor_order = 3; // Default predictor order
    int num_bits = 16;       // Default number of bits for lossy

    if (!isLossless) {
        cout << "Enter desired bitrate (number of bits): ";
        cin >> num_bits;
    }

    if (dynamicM) {
        M = calculate_dynamic_m(buffer, frames, channels, isLossless, predictor_order, num_bits);
    }

    cout << "Using M: " << M << endl;

    if (isLossless) {
        cout << "Performing lossless encoding...\n";
        perform_lossless_encoding(buffer, frames, M, predictor_order, sample_rate, channels);
    } else {
        cout << "Performing lossy encoding...\n";
        perform_lossy_encoding(buffer, frames, M, predictor_order, num_bits, sample_rate, channels);
    }

    sf_close(sf);
    delete[] buffer;
    return 0;
}
