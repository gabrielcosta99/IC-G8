#include<stdio.h>
#include <fstream>
#include <iostream>
#include "./t1.cpp"
#include "./t2.cpp"
using namespace std;



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
        // std::cerr << "Warning: Noise power is zero. SNR is infinite." << std::endl;
        return std::numeric_limits<double>::infinity();
    }

    // signal_power /= original.size();
    // noise_power /= original.size();

    return 10 * log10(signal_power / noise_power);
}



int predict_sample(const int16_t* buffer,int channels, int n, int order) {
    if (order == 0) return 0;

    // Adjust indexing for stereo
    int step = channels; // Each frame has two samples (left and right) if it is stereo

    if (order == 1) return buffer[n - step ];
    if (order == 2) return 2 * buffer[n - step] - buffer[n - 2 * step];
    if (order == 3) return 3 * buffer[n - step] - 3 * buffer[n - 2 * step] + buffer[n - 3 * step];
    if (order == 4) return buffer[n - step] + (buffer[n - step] - buffer[n - 2*step]);
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
            if(channels == 2){
                prediction_errors.push_back(buffer[i * 2]);     // Left channel
                prediction_errors.push_back(buffer[i * 2 + 1]); // Right channel
            }
            else
                prediction_errors.push_back(buffer[i]);     // Left channel
        }
        for (int i = predictor_order; i < frames; i++) {
            if(channels == 2){
                int left_idx = i * 2;
                int right_idx = i * 2 + 1;

                int16_t prediction_left = predict_sample(buffer, channels,left_idx, predictor_order);
                int16_t residual_left = buffer[left_idx] - prediction_left;
                prediction_errors.push_back(abs(residual_left));

                int16_t prediction_right = predict_sample(buffer, channels, right_idx, predictor_order);
                int16_t residual_right = buffer[right_idx] - prediction_right;
                prediction_errors.push_back(abs(residual_right));
            }
            else{
                int16_t prediction = predict_sample(buffer, channels,i, predictor_order);
                int16_t residual = buffer[i] - prediction;
                prediction_errors.push_back(abs(residual));
            }
        }
    }
    else{
        int16_t *buffercpy = new int16_t[frames * channels];   
        memcpy(buffercpy,buffer,frames*channels*sizeof(int16_t));  // make a copy of the buffer with the original data

        for (int i = 0; i < predictor_order; i++) {
            // quantized the samples to have the "num_bits" data rate
            if(channels == 2){
                int16_t left_sample = quantize_sample(buffer[i * 2],num_bits);      
                int16_t right_sample = quantize_sample(buffer[i * 2 + 1],num_bits);

                prediction_errors.push_back(left_sample);     // Left channel
                prediction_errors.push_back(right_sample); // Right channel

                // save the samples after reversing the quantization in the buffer, in order to have the same samples in the encoder and decoder
                buffercpy[i*2] = dequantize_sample(left_sample,num_bits);           
                buffercpy[i*2+1] = dequantize_sample(right_sample,num_bits);
            }
            else{
                int16_t sample = quantize_sample(buffer[i],num_bits);      
                prediction_errors.push_back(sample); 
                buffercpy[i] = dequantize_sample(sample,num_bits);           
            }
        }
        // Encode residuals with prediction
        for (int i = predictor_order; i < frames; i++) {
            if(channels == 2) {
                int left_idx = i * 2;
                int right_idx = i * 2 + 1;

                int16_t prediction_left = predict_sample(buffercpy, channels, left_idx, predictor_order);
                int16_t residual_left = buffercpy[left_idx] - prediction_left;
                residual_left = quantize_sample(residual_left,num_bits);
                prediction_errors.push_back(abs(residual_left));
                buffercpy[left_idx] = prediction_left+dequantize_sample( residual_left,num_bits);     
                
                int16_t prediction_right = predict_sample(buffercpy, channels, right_idx, predictor_order);
                int16_t residual_right = buffercpy[right_idx] - prediction_right;
                residual_right = quantize_sample(residual_right,num_bits);
                buffercpy[right_idx] = prediction_right+dequantize_sample(residual_right,num_bits);     
                prediction_errors.push_back(abs(residual_right));
            }
            else{
                int16_t prediction = predict_sample(buffercpy, channels, i, predictor_order);
                int16_t residual = buffercpy[i] - prediction;
                residual = quantize_sample(residual,num_bits);
                prediction_errors.push_back(abs(residual));
                buffercpy[i] = prediction+dequantize_sample( residual,num_bits);     
            }
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







int main(int argc, char* argv[]){
    if (argc < 2) {
        cout << "Usage: ./main <audio_file>" << endl;
        cout << "Example: ./main audio_samples/sample01.wav" << endl;
        return 1;
    }
    const char* filename = argv[1];

    // const char* filename = argv[1];
    SF_INFO sfinfo = { 0 };
    SNDFILE* sf = sf_open(filename, SFM_READ, &sfinfo);
    cout << sf_error(sf) << endl;
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
    // ask if the user wants to give an M value for the Golomb (default 'N')
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
    int num_bits = 16;       // Default bitrate for lossy

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