#include <iostream>
#include <sndfile.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;
namespace fs = filesystem;

// Function to plot waveform using gnuplot
void plot_waveform(const char* data_filename, const char* title) {
    FILE* gnuplotPipe = popen("gnuplot -persistent", "w");

    if (!gnuplotPipe) {
        cerr << "Error: Could not open pipe to gnuplot." << endl;
        return;
    }

    // Setup gnuplot to plot the waveform
    fprintf(gnuplotPipe, "set title '%s'\n", title);
    fprintf(gnuplotPipe, "set xlabel 'Sample Index'\n");
    fprintf(gnuplotPipe, "set ylabel 'Amplitude'\n");
    fprintf(gnuplotPipe, "plot '%s' using 1:2 with lines notitle\n", data_filename);
    fflush(gnuplotPipe);

    // Wait for user input to close the plot
    cout << "Press Enter to close the plot for " << title << "...";
    cin.ignore();

    // Close the pipe
    pclose(gnuplotPipe);
}

// Function to save waveform data for plotting
void save_waveform_data(const vector<float>& data, const char* filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    for (size_t i = 0; i < data.size(); ++i) {
        file << i << " " << data[i] << endl;
    }

    file.close();
}

// Quantize samples to a specified number of bits
float quantize_sample(float sample, int num_bits) {
    int levels = (1 << num_bits) - 1; // Number of quantization levels
    sample = (sample + 1.0f) / 2.0f;  // Normalize to [0, 1]
    sample = round(sample * levels) / levels; // Quantize
    sample = (sample * 2.0f) - 1.0f;  // Rescale to [-1, 1]
    return sample;
}

// Quantize the entire audio buffer
vector<float> quantize_audio(const vector<float>& audio_data, int num_bits) {
    vector<float> quantized_data(audio_data.size());
    for (size_t i = 0; i < audio_data.size(); ++i) {
        quantized_data[i] = quantize_sample(audio_data[i], num_bits);
    }
    return quantized_data;
}

// Save quantized audio to a new WAV file
void save_quantized_wav(const char* output_filename, const vector<float>& quantized_data, int sample_rate, int channels) {
    SF_INFO sfinfo;
    sfinfo.samplerate = sample_rate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sf_out = sf_open(output_filename, SFM_WRITE, &sfinfo);
    if (!sf_out) {
        cerr << "Error: Could not open output WAV file." << endl;
        return;
    }

    sf_writef_float(sf_out, quantized_data.data(), quantized_data.size() / channels);
    sf_close(sf_out);
}

// Calculate the Mean Squared Error (MSE)
float calculate_mse(const vector<float>& original, const vector<float>& quantized) {
    float mse = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) {
        float diff = original[i] - quantized[i];
        mse += diff * diff;
    }
    return mse / original.size();
}

// Calculate Signal-to-Noise Ratio (SNR)
float calculate_snr(const vector<float>& original, const vector<float>& quantized) {
    float signal_power = 0.0f;
    float noise_power = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) {
        signal_power += original[i] * original[i];
        float noise = original[i] - quantized[i];
        noise_power += noise * noise;
    }
    if (noise_power == 0) return INFINITY; // No noise introduced
    return 10.0f * log10(signal_power / noise_power); // SNR in decibels (dB)
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <input sound file> <bit depth>" << endl;
        return 1;
    }

    const char* filename = argv[1];
    int bit_depth = atoi(argv[2]); // Number of bits for quantization
    SF_INFO sfinfo;

    SNDFILE* sf = sf_open(filename, SFM_READ, &sfinfo);
    if (!sf) {
        cout << "Error opening the file" << endl;
        return 1;
    }

    int sample_rate = sfinfo.samplerate;
    int channels = sfinfo.channels;
    int frames = sfinfo.frames;

    if (channels != 2) {
        cout << "This example is designed for stereo (2 channels) audio files." << endl;
        sf_close(sf);
        return 1;
    }

    int numSamples = frames * channels;
    float* buffer = new float[numSamples];

    sf_readf_float(sf, buffer, frames);

    // Vectors to hold original and quantized data
    vector<float> original_audio(buffer, buffer + numSamples);
    vector<float> quantized_audio = quantize_audio(original_audio, bit_depth);

    // Calculate MSE and SNR
    float mse = calculate_mse(original_audio, quantized_audio);
    float snr = calculate_snr(original_audio, quantized_audio);

    cout << "Mean Squared Error (MSE): " << mse << endl;
    cout << "Signal-to-Noise Ratio (SNR): " << snr << " dB" << endl;

    // Save waveform data for both original and quantized audio for plotting
    save_waveform_data(original_audio, "original_waveform.dat");
    save_waveform_data(quantized_audio, "quantized_waveform.dat");

    // Plot both waveforms
    plot_waveform("original_waveform.dat", "Original Audio Waveform");
    plot_waveform("quantized_waveform.dat", "Quantized Audio Waveform");

    // Save the quantized audio as a new WAV file
    save_quantized_wav("quantized_output.wav", quantized_audio, sample_rate, channels);

    // Cleanup
    sf_close(sf);
    delete[] buffer;

    cout << "Quantized audio saved to 'quantized_output.wav'" << endl;

    return 0;
}
