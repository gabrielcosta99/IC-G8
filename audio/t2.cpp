#include <iostream>
#include <sndfile.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

void plot_waveform(const char* data_filename, const char* channel_name, int sampleRate) {
    // Open a pipe to gnuplot
    FILE* gnuplotPipe = popen("gnuplot -persistent", "w");

    if (!gnuplotPipe) {
        cerr << "Error: Could not open pipe to gnuplot." << endl;
        return;
    }

    // Setup gnuplot to plot the waveform
    fprintf(gnuplotPipe, "set title '%s Channel Waveform'\n", channel_name);
    fprintf(gnuplotPipe, "set xlabel 'Time (seconds)'\n");
    fprintf(gnuplotPipe, "set ylabel 'Amplitude'\n");
    fprintf(gnuplotPipe, "plot '%s' using 1:2 with lines title '%s Channel'\n", data_filename, channel_name);
    fflush(gnuplotPipe);

    // Wait for user input to close the plot
    cout << "Press Enter to close the plot for " << channel_name << " channel...";
    cin.ignore();

    // Close the pipe
    pclose(gnuplotPipe);
}

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

    // Open files to write the left and right channel data
    ofstream leftFile("left_channel.dat");
    ofstream rightFile("right_channel.dat");

    if (!leftFile.is_open() || !rightFile.is_open()) {
        cerr << "Error opening output files." << endl;
        sf_close(sf);
        delete[] buffer;
        return 1;
    }

    // Write time (in seconds) and amplitude data for left and right channels
    for (int i = 0; i < frames; i++) {
        float time = static_cast<float>(i) / sample_rate;
        float left_sample = buffer[i * 2];      // Left channel
        float right_sample = buffer[i * 2 + 1]; // Right channel

        leftFile << time << " " << left_sample << endl;
        rightFile << time << " " << right_sample << endl;
    }

    // Close files and cleanup
    leftFile.close();
    rightFile.close();

    sf_close(sf);
    delete[] buffer;

    // Plot the waveforms for both channels
    plot_waveform("left_channel.dat", "Left", sample_rate);
    plot_waveform("right_channel.dat", "Right", sample_rate);

    return 0;
}
