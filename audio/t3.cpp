#include <iostream>
#include <sndfile.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono> // Include chrono for timing

using namespace std;
namespace fs = filesystem;
using namespace chrono;

// Function to create improved exponential bins
vector<int> create_exponential_bins(const vector<float>& data, int num_bins) {
    vector<int> histogram(num_bins, 0);

    // Calculate bin boundaries
    float min_val = *min_element(data.begin(), data.end());
    float max_val = *max_element(data.begin(), data.end());

    // Calculate refined exponential bin ranges
    vector<float> bin_boundaries(num_bins + 1);
    for (int i = 0; i <= num_bins; ++i) {
        bin_boundaries[i] = min_val + (max_val - min_val) * (pow(2.0f, float(i) / num_bins) - 1) / (pow(2.0f, 1.0f) - 1);
    }

    // Place data into bins
    for (float sample : data) {
        int bin_index = upper_bound(bin_boundaries.begin(), bin_boundaries.end(), sample) - bin_boundaries.begin() - 1;
        bin_index = min(bin_index, num_bins - 1);
        histogram[bin_index]++;
    }

    // Apply smoothing for better appearance
    for (int i = 1; i < num_bins - 1; ++i) {
        histogram[i] = (histogram[i - 1] + histogram[i] + histogram[i + 1]) / 3;
    }

    return histogram;
}

// Updated function to save histogram data with amplitude values as x-axis
void save_histogram_data(const vector<int>& histogram, const vector<float>& bin_boundaries, const char* filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    // Save histogram data (amplitude value and count)
    for (size_t i = 0; i < histogram.size(); ++i) {
        file << bin_boundaries[i] << " " << histogram[i] << endl;
    }

    file.close();
}

// Updated plot function to use amplitude as x-axis and frequency as y-axis
void plot_histogram(const char* data_filename, const char* title) {
    FILE* gnuplotPipe = popen("gnuplot -persistent", "w");

    if (!gnuplotPipe) {
        cerr << "Error: Could not open pipe to gnuplot." << endl;
        return;
    }

    // Configure gnuplot for logarithmic y-axis (frequency)
    fprintf(gnuplotPipe, "set title '%s'\n", title);
    fprintf(gnuplotPipe, "set xlabel 'Amplitude'\n");
    fprintf(gnuplotPipe, "set ylabel 'Frequency (log scale)'\n");
    fprintf(gnuplotPipe, "set logscale y\n");  // Logarithmic scaling for y-axis

    // Style adjustments for better readability
    fprintf(gnuplotPipe, "set style fill solid 0.5 border -1\n");
    fprintf(gnuplotPipe, "set boxwidth 0.9 relative\n");
    fprintf(gnuplotPipe, "set grid ytics lc rgb '#bbbbbb' lw 1 lt 0\n");
    fprintf(gnuplotPipe, "set xtics rotate by -45\n");

    // Plot histogram with amplitude values on x-axis
    fprintf(gnuplotPipe, "plot '%s' using 1:2 with boxes notitle\n", data_filename);

    fflush(gnuplotPipe);
    cout << "Press Enter to close the plot for " << title << "...";
    cin.ignore();

    pclose(gnuplotPipe);
}

// Updated main function to use new save_histogram_data with amplitude bin boundaries
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Please provide a sound file as an argument." << endl;
        return 1;
    }
    auto start = high_resolution_clock::now(); // Start timing

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

    // Collect amplitude values for left, right, mid, and side channels
    vector<float> left_channel, right_channel, mid_channel, side_channel;

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

    // Parameters for histogram
    int num_bins = 32;  // Increased number of bins for more amplitude detail

    // Create exponential bins and bin boundaries
    vector<int> left_histogram = create_exponential_bins(left_channel, num_bins);
    vector<int> right_histogram = create_exponential_bins(right_channel, num_bins);
    vector<int> mid_histogram = create_exponential_bins(mid_channel, num_bins);
    vector<int> side_histogram = create_exponential_bins(side_channel, num_bins);

    // Generate bin boundaries once (common across channels)
    float min_val = *min_element(left_channel.begin(), left_channel.end());
    float max_val = *max_element(left_channel.begin(), left_channel.end());
    vector<float> bin_boundaries(num_bins + 1);
    for (int i = 0; i <= num_bins; ++i) {
        bin_boundaries[i] = min_val + (max_val - min_val) * (pow(2.0f, float(i) / num_bins) - 1) / (pow(2.0f, 1.0f) - 1);
    }

    auto end = high_resolution_clock::now(); // End timing

    // Calculate duration in milliseconds
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Total execution time: " << duration.count() << " ms" << endl;

    // Save histograms to files
    save_histogram_data(left_histogram, bin_boundaries, "left_histogram.dat");
    save_histogram_data(right_histogram, bin_boundaries, "right_histogram.dat");
    save_histogram_data(mid_histogram, bin_boundaries, "mid_histogram.dat");
    save_histogram_data(side_histogram, bin_boundaries, "side_histogram.dat");

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
