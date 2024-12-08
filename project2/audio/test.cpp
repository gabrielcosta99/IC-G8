#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstring>
#include "../Golomb.h"
#define BUFFER_SIZE 1024  // Size of processing buffer

// Simple linear predictor: predict next sample based on the previous one
int16_t predict_sample(int16_t prev_sample) {
    return prev_sample;  // Predict the next sample is the same as the previous
}

// Compress raw PCM audio using predictive coding
void compress_audio(const char *input_file, const char *output_file) {
    Golomb g(4096,false);
    FILE *in = fopen(input_file, "rb");
    FILE *out = fopen(output_file, "wb");

    if (!in || !out) {
        perror("File error");
        return;
    }

    int16_t prev_sample = 0;  // Initialize previous sample
    int16_t sample;           // Current audio sample
    int16_t error;            // Prediction error

    // Process audio samples
    while (fread(&sample, sizeof(int16_t), 1, in) == 1) {
        // Predict the current sample
        int16_t predicted = predict_sample(prev_sample);

        // Calculate prediction error
        error = sample - predicted;
        // printf("error: %d\n",error);
        // Write error to the output file
        // fwrite(&error, sizeof(int16_t), 1, out);
        g.encode_val(error);
        // Update the previous sample
        prev_sample = sample;
    }

    fclose(in);
    fclose(out);
    g.end();
}

// Decompress audio using predictive coding
void decompress_audio(const char *compressed_file, const char *output_file) {
    Golomb g(4096,true);
    FILE *in = fopen(compressed_file, "rb");
    FILE *out = fopen(output_file, "wb");

    if (!in || !out) {
        perror("File error");
        return;
    }

    int16_t prev_sample = 0;  // Initialize previous sample
    int16_t error;           // Prediction error
    int16_t reconstructed;   // Reconstructed sample
    vector<int> values = g.decode(); 

    for(int i = 0; i< values.size(); i++){
        error = values[i];
        // Reconstruct the sample
        reconstructed = predict_sample(prev_sample) + error;

        // Write the reconstructed sample to the output file
        fwrite(&reconstructed, sizeof(int16_t), 1, out);
        
        // Update the previous sample
        prev_sample = reconstructed;
    }

    // // Process compressed error values
    // while (fread(&error, sizeof(int16_t), 1, in) == 1) {
    //     // Reconstruct the sample
    //     reconstructed = predict_sample(prev_sample) + error;

    //     // Write the reconstructed sample to the output file
    //     fwrite(&reconstructed, sizeof(int16_t), 1, out);
        
    //     // Update the previous sample
    //     prev_sample = reconstructed;
    // }
    g.end();
    fclose(in);
    fclose(out);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <compress|decompress> <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "compress") == 0) {
        compress_audio(argv[2], argv[3]);
    } else if (strcmp(argv[1], "decompress") == 0) {
        decompress_audio(argv[2], argv[3]);
    } else {
        printf("Invalid option: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
