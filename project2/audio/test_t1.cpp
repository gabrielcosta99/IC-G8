#include <iostream>
#include <sndfile.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <chrono> // Include chrono for timing
#include "../Golomb.h"


#define M_VALUE 32768

using namespace std;
namespace fs = filesystem;
using namespace chrono;

void save_wav(const char* output_filename, const vector<int>& data, int sample_rate, int channels) {
    SF_INFO sfinfo;
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

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "Please provide a sound file as an argument." << endl;
        return 1;
    }

    const char *filename = argv[1];
    SF_INFO sfinfo;

    SNDFILE *sf = sf_open(filename, SFM_READ, &sfinfo);
    if (!sf)
    {
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

    if (channels != 2)
    {
        cout << "This example is designed for stereo (2 channels) audio files." << endl;
        sf_close(sf);
        return 1;
    }
    auto start = high_resolution_clock::now(); // Start timing

    int numSamples = frames * channels;
    int *buffer = new int[numSamples];

    sf_readf_int(sf, buffer, frames);

    // Open files to write the left and right channel data
    // ofstream leftFile("left_channel.dat");
    // ofstream rightFile("right_channel.dat");

    // if (!leftFile.is_open() || !rightFile.is_open())
    // {
    //     cerr << "Error opening output files." << endl;
    //     sf_close(sf);
    //     delete[] buffer;
    //     return 1;
    // }
    printf("Encoding the wav file\n");
    Golomb encoder(M_VALUE,false,"error.bin");
    // Golomb right_channel(M_VALUE,false,"right_error.bin");
    int previous_left_sample = buffer[0];
    int previous_right_sample = buffer[1];

    encoder.encode_val(previous_left_sample);
    encoder.encode_val(previous_right_sample);
    // Write time (in seconds) and amplitude data for left and right channels
    for (int i = 1; i < frames; i++)
    {
        printf("i:%d\n",i);
        int left_sample = buffer[i * 2];      // Left channel
        int right_sample = buffer[i * 2 + 1]; // Right channel

        // Compute the error using the previous sample
        int left_error = left_sample - previous_left_sample;
        int right_error = right_sample - previous_right_sample;

        // printf("left:%d, previous_left:%d, left_error: %d, right_error: %d\n",left_sample,previous_left_sample,left_error,right_error);

        // Save the error using Golomb
        encoder.encode_val(left_error);
        encoder.encode_val(right_error);

        // update the values
        previous_left_sample = left_sample;
        previous_right_sample = right_sample;
    }
    encoder.end();

    printf("Decoding the wav file\n");
    Golomb decoder(M_VALUE,true,"error.bin");

    vector<int> errors = decoder.decode();
    decoder.end();

    vector<int> decoded_values;
    previous_left_sample = errors[0]; // Initial value
    previous_right_sample = errors[1]; // Initial value
    decoded_values.push_back(previous_left_sample);
    decoded_values.push_back(previous_right_sample);
    int i = 2;
    for (int error : errors) {
        if(i%2==0){
            int original_sample = error + previous_left_sample; // Reconstruct sample
            decoded_values.push_back(original_sample);
            previous_left_sample = original_sample;
        }
        else{
            int original_sample = error + previous_right_sample; // Reconstruct sample
            decoded_values.push_back(original_sample);
            previous_right_sample = original_sample;
        }
        i++;
    }
    
    // vector<int> stereo_data;
    // for (size_t i = 0; i < decoded.size(); ++i) {
    //     stereo_data.push_back(left_channel_decoded_values[i]); // Left channel
    //     stereo_data.push_back(right_channel_decoded_values[i]); // Right channel
    // }
    save_wav("reconstructed_stereo.wav", decoded_values, sample_rate, 2);

    auto end = high_resolution_clock::now(); // End timing

    // Calculate duration in milliseconds
    auto durationt = duration_cast<milliseconds>(end - start);
    cout << "Total execution time: " << durationt.count() << " ms" << endl;

    // Close files and cleanup
    // leftFile.close();
    // rightFile.close();

    sf_close(sf);
    delete[] buffer;
    return 0;
}
