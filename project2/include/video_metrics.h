#pragma once
#include <opencv2/opencv.hpp>

struct VideoMetrics {
    double compressionRatio;
    double encodingTimePerFrame;
    double decodingTimePerFrame;
    size_t originalSize;
    size_t compressedSize;
    double averagePSNR;
    int totalFrames;
    // For inter-frame specific metrics
    double averageMotionVectorBits;
    double iFrameCompressionRatio;
    double pFrameCompressionRatio;
    
    // Optional lossy-specific metrics
    double averageQuantizationError;
};

class VideoAnalyzer {
public:
    static double calculatePSNR(const cv::Mat& original, const cv::Mat& compressed) {
        cv::Mat diff;
        cv::absdiff(original, compressed, diff);
        diff.convertTo(diff, CV_64F);
        diff = diff.mul(diff);
        double mse = cv::mean(diff)[0];
        if(mse <= 1e-10) return std::numeric_limits<double>::infinity();
        return 10.0 * log10((255 * 255) / mse);
    }

    static size_t getFileSize(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        return file.tellg();
    }
};
