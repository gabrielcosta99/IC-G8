#include <stdint.h>
#include <vector>
#include <cmath>

int predict_sample(const int16_t* buffer, int channels, int n, int order);
int16_t quantize_sample(int16_t sample, int num_bits);
int16_t dequantize_sample(int16_t sample, int num_bits);
int calculate_dynamic_m(int16_t* buffer, int frames, int channels, bool isLossless, int predictor_order, int num_bits);
void save_wav(const char* output_filename, const vector<int16_t>& data, int sample_rate, int channels);
double calculate_snr(const std::vector<int16_t>& original, const std::vector<int16_t>& reconstructed);

