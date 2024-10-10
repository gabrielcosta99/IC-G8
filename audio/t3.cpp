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

// Function to plot histogram using gnuplot
void plot_histogram(const char* data_filename, const char* title) {
    FILE* gnuplotPipe = popen("gnuplot -persistent", "w");

    if (!gnuplotPipe) {
        cerr << "Error: Could not open pipe to gnuplot." << endl;
        return;
    }

    // Setup gnuplot to plot the histogram
    fprintf(gnuplotPipe, "set title '%s'\n", title);
    fprintf(gnuplotPipe, "set xlabel 'Amplitude'\n");
    fprintf(gnuplotPipe, "set ylabel 'Frequency'\n");
    fprintf(gnuplotPipe, "set style fill solid\n");
    fprintf(gnuplotPipe, "plot '%s' using 1:2 with boxes notitle\n", data_filename);
    fflush(gnuplotPipe);

    // Wait for user input to close the plot
    cout << "Press Enter to close the plot for " << title << "...";
    cin.ignore();

    // Close the pipe
    pclose(gnuplotPipe);
}

// Function to normalize data to [-1, 1]
vector<float> normalize_data(const vector<float>& data) {
    float max_val = *max_element(data.begin(), data.end());
    float min_val = *min_element(data.begin(), data.end());
    float range = max(max_val, -min_val);  // The maximum absolute value

    vector<float> normalized_data(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        normalized_data[i] = data[i] / range;
    }
    return normalized_data;
}

// Create histogram with bins increasing in powers of 2
vector<int> create_logarithmic_histogram(const vector<float>& data, int num_bins) {
    vector<int> histogram(num_bins, 0);

    // Assuming normalized data in range [-1, 1]
    for (float sample : data) {
        // Map the sample to a bin index
        float normalized_sample = fabs(sample);  // Use absolute value for symmetric bins
        int bin_index = (normalized_sample == 0.0f) ? 0 : static_cast<int>(log2(normalized_sample * (1 << (num_bins - 1))));
        
        // Safeguard against any possible overflow or negative indices
        bin_index = min(max(bin_index, 0), num_bins - 1);
        histogram[bin_index]++;
    }

    return histogram;
}

// Save histogram data to file
void save_histogram_data(const vector<int>& histogram, const char* filename, int num_bins) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    // Save bin index and histogram count (logarithmic binning)
    for (int i = 0; i < num_bins; ++i) {
        file << pow(2, i) << " " << histogram[i] << endl;
    }

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Please provide a sound file as an argument." << endl;
        return 1;
    }

    const char* filename = argv[1];
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

    // Vectors to hold amplitude data for each channel
    vector<float> left_channel;
    vector<float> right_channel;
    vector<float> mid_channel;
    vector<float> side_channel;

    // Collect amplitude values for left, right, mid, and side channels
    for (int i = 0; i < frames; i++) {
        float left_sample = buffer[i * 2];      // Left channel
        float right_sample = buffer[i * 2 + 1]; // Right channel

        left_channel.push_back(left_sample);
        right_channel.push_back(right_sample);

        // Calculate mid (L + R) / 2 and side (L - R) / 2 channels
        float mid_sample = (left_sample + right_sample) / 2.0f;
        float side_sample = (left_sample - right_sample) / 2.0f;

        mid_channel.push_back(mid_sample);
        side_channel.push_back(side_sample);
    }

    // Normalize the channels to [-1, 1]
    left_channel = normalize_data(left_channel);
    right_channel = normalize_data(right_channel);
    mid_channel = normalize_data(mid_channel);
    side_channel = normalize_data(side_channel);

    // Parameters for histogram
    int num_bins = 10; // Logarithmic bins

    // Create histograms with logarithmic binning
    vector<int> left_histogram = create_logarithmic_histogram(left_channel, num_bins);
    vector<int> right_histogram = create_logarithmic_histogram(right_channel, num_bins);
    vector<int> mid_histogram = create_logarithmic_histogram(mid_channel, num_bins);
    vector<int> side_histogram = create_logarithmic_histogram(side_channel, num_bins);

    // Save histogram data to files
    save_histogram_data(left_histogram, "left_histogram.dat", num_bins);
    save_histogram_data(right_histogram, "right_histogram.dat", num_bins);
    save_histogram_data(mid_histogram, "mid_histogram.dat", num_bins);
    save_histogram_data(side_histogram, "side_histogram.dat", num_bins);

    // Plot the histograms for left, right, mid, and side channels
    plot_histogram("left_histogram.dat", "Left Channel Amplitude Histogram");
    plot_histogram("right_histogram.dat", "Right Channel Amplitude Histogram");
    plot_histogram("mid_histogram.dat", "Mid Channel Amplitude Histogram");
    plot_histogram("side_histogram.dat", "Side Channel Amplitude Histogram");

    // Cleanup
    sf_close(sf);
    delete[] buffer;

    return 0;
}
