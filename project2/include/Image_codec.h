//#ifdef IMAGE_CODEC_H
//#define IMAGE_CODEC_H

#pragma once

#include <vector>
#include <cmath> // For ceil, log2
#include <iostream>
#include "Golomb.h" // Include the Golomb class for encoding/decoding
#include <opencv2/opencv.hpp>
#include <filesystem> // For filesystem operations
#include <chrono> // For measuring encoding time

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

class ImageCodec {
private:
    int m;      // Golomb parameter
    int mode;   // Prediction mode (0: A, 1: Average, etc.)
    int channelsCount; // Number of channels in the image

    struct PredictorMetrics {
        double psnr;
        double compressionRatio;
        double encodingTime;
        double meanResidual;
        int optimalM;
    };

    /*
    * Calculate the A predictor for a single channel
    */
    int predictorA(int x, int y, const Mat &channel) {
        return (x > 0) ? channel.at<uchar>(y, x - 1) : 0;
    }

    // Add more JPEG-LS predictors
    int predictorB(int x, int y, const Mat &channel) {
        return (y > 0) ? channel.at<uchar>(y - 1, x) : 0;  // North
    }

    int predictorC(int x, int y, const Mat &channel) {
        return (x > 0 && y > 0) ? channel.at<uchar>(y - 1, x - 1) : 0;  // Northwest
    }

    // JPEG-LS predictor
    int JPEGLSPredictor(int x, int y, const Mat &channel) {
        if (x == 0 || y == 0) return predictorA(x, y, channel);
        
        int a = channel.at<uchar>(y, x - 1);     // West
        int b = channel.at<uchar>(y - 1, x);     // North
        int c = channel.at<uchar>(y - 1, x - 1); // Northwest

        // JPEG-LS predictor logic
        if (c >= max(a, b))
            return min(a, b);
        else if (c <= min(a, b))
            return max(a, b);
        else
            return a + b - c;
    }

    // Optimal predictor selection
    int predictPixel(int x, int y, const Mat &channel) {
        if (x == 0 && y == 0) return 128;  // Default value for first pixel
        if (x == 0) return predictorB(x, y, channel);  // First column: use north
        if (y == 0) return predictorA(x, y, channel);  // First row: use west
        
        return JPEGLSPredictor(x, y, channel);  // Use JPEG-LS predictor for other pixels
    }

    // Estimate optimal Golomb parameter m
    int estimateOptimalM(const Mat &residuals) {
        // Calculate mean absolute value of residuals
        double sum = 0;
        for (int y = 0; y < residuals.rows; ++y) {
            for (int x = 0; x < residuals.cols; ++x) {
                sum += abs(residuals.at<int>(y, x));
            }
        }
        double mean = sum / (residuals.rows * residuals.cols);
        
        // Estimate optimal m based on mean (Rice parameter selection)
        return max(1, static_cast<int>(ceil(-1/log2(mean/(mean+1)))));
    }

    // New helper method to calculate PSNR
    double calculatePSNR(const Mat& original, const Mat& reconstructed) {
        // For lossless compression, first check if images are identical
        Mat diff;
        absdiff(original, reconstructed, diff);
        double maxDiff = norm(diff, NORM_INF);
        
        if (maxDiff < 1e-6) {
            return std::numeric_limits<double>::infinity();
        }
        
        // If there are differences (shouldn't happen in lossless), calculate PSNR
        diff.convertTo(diff, CV_64F);
        diff = diff.mul(diff);
        double mse = mean(diff)[0];
        return 10.0 * log10((255 * 255) / mse);
    }

    // Method to evaluate a single predictor
    PredictorMetrics evaluatePredictor(const Mat& channel, int predictorType) {
        PredictorMetrics metrics;
        auto start = chrono::high_resolution_clock::now();
        
        // Calculate residuals using specified predictor
        Mat residuals = Mat::zeros(channel.size(), CV_32S);
        Mat reconstructed = Mat::zeros(channel.size(), CV_8UC1);

        // First pass: calculate residuals and reconstruct simultaneously
        for (int y = 0; y < channel.rows; ++y) {
            for (int x = 0; x < channel.cols; ++x) {
                int predicted;
                // Use reconstructed values for prediction
                switch(predictorType) {
                    case 0: // West
                        predicted = (x > 0) ? reconstructed.at<uchar>(y, x-1) : 0;
                        break;
                    case 1: // North
                        predicted = (y > 0) ? reconstructed.at<uchar>(y-1, x) : 0;
                        break;
                    case 2: // Northwest
                        predicted = (x > 0 && y > 0) ? reconstructed.at<uchar>(y-1, x-1) : 0;
                        break;
                    case 3: // JPEG-LS
                        if (x == 0 || y == 0) {
                            predicted = (x > 0) ? reconstructed.at<uchar>(y, x-1) : 
                                      (y > 0) ? reconstructed.at<uchar>(y-1, x) : 0;
                        } else {
                            int a = reconstructed.at<uchar>(y, x-1);     // West
                            int b = reconstructed.at<uchar>(y-1, x);     // North
                            int c = reconstructed.at<uchar>(y-1, x-1);   // Northwest
                            if (c >= max(a, b))
                                predicted = min(a, b);
                            else if (c <= min(a, b))
                                predicted = max(a, b);
                            else
                                predicted = a + b - c;
                        }
                        break;
                    default:
                        predicted = 128;
                }
                
                int current = channel.at<uchar>(y, x);
                residuals.at<int>(y, x) = current - predicted;
                reconstructed.at<uchar>(y, x) = saturate_cast<uchar>(predicted + residuals.at<int>(y, x));
            }
        }
        
        // Calculate metrics
        metrics.meanResidual = 0;
        for(int y = 0; y < residuals.rows; y++) {
            for(int x = 0; x < residuals.cols; x++) {
                metrics.meanResidual += abs(residuals.at<int>(y, x));
            }
        }
        metrics.meanResidual /= (residuals.rows * residuals.cols);
        metrics.optimalM = estimateOptimalM(residuals);
        
        // Verify perfect reconstruction
        Mat diff;
        absdiff(channel, reconstructed, diff);
        double maxDiff = norm(diff, NORM_INF);
        metrics.psnr = (maxDiff == 0) ? std::numeric_limits<double>::infinity() : 
                      20 * log10(255.0 / maxDiff);
        
        // Calculate compression ratio
        double originalBits = channel.rows * channel.cols * 8;
        double compressedBits = 0;
        
        // Estimate bits without using Golomb class directly
        for(int y = 0; y < residuals.rows; y++) {
            for(int x = 0; x < residuals.cols; x++) {
                int value = abs(residuals.at<int>(y, x));
                // Estimate bits for quotient and remainder
                int q = value / metrics.optimalM;
                compressedBits += q + 1; // unary code for quotient
                compressedBits += ceil(log2(metrics.optimalM)); // bits for remainder
                compressedBits += 1; // sign bit
            }
        }
        metrics.compressionRatio = compressedBits / originalBits;
        
        // Calculate encoding time
        auto end = chrono::high_resolution_clock::now();
        metrics.encodingTime = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        
        return metrics;
    }

public:
    int getM() const { return m; }
    

    /*
    * Calculate residuals for a single channel using the A predictor
    */
    Mat calculateResiduals(const Mat &channel) {
        Mat residuals = Mat::zeros(channel.size(), CV_32S);
        for (int y = 0; y < channel.rows; ++y) {
            for (int x = 0; x < channel.cols; ++x) {
                int predicted = predictPixel(x, y, channel);
                residuals.at<int>(y, x) = channel.at<uchar>(y, x) - predicted;
            }
        }
        return residuals;
    }

    /*
    * Reconstruct a single channel using residuals and the JPEG-LS predictor
    */
    Mat reconstructChannel(const Mat &residuals) {
        Mat channel = Mat::zeros(residuals.size(), CV_8U);
        for (int y = 0; y < residuals.rows; ++y) {
            for (int x = 0; x < residuals.cols; ++x) {
                int predicted = predictPixel(x, y, channel); // Use same predictor as encoding
                int value = residuals.at<int>(y, x) + predicted;
                channel.at<uchar>(y, x) = saturate_cast<uchar>(value);
            }
        }
        return channel;
    }

    /*
     * Encode residuals using Golomb coding and save to a binary file
     * @param residuals Vector of residual matrices (one per channel)
     * @param outputFilename Output file to store the encoded data
     */
    void encodeResiduals(const vector<Mat> &residuals, const string &outputFilename) {
        Golomb encoder(m, false, outputFilename); // Initialize Golomb encoder
        // Encode each channel sequentially in the same file
        for (const Mat &channelResiduals : residuals) {
            for (int y = 0; y < channelResiduals.rows; ++y) {
                for (int x = 0; x < channelResiduals.cols; ++x) {
                    encoder.encode(channelResiduals.at<int>(y, x));
                }
            }
        }
        
        encoder.end(); // Finalize and close the file
    }

    /*
     * Decode residuals from a binary file using Golomb coding
     * @param width Width of the image
     * @param height Height of the image
     * @param channels Number of channels
     * @param inputFilename Input file with encoded data
     * @return Vector of decoded residuals matrices
     */
    vector<Mat> decodeResiduals(int width, int height, int channels, const string &inputFilename) {
        Golomb decoder(m, true, inputFilename); // Initialize Golomb decoder
        vector<Mat> residuals(channels);
        
        // Initialize matrices for each channel
        for (int c = 0; c < channels; ++c) {
            residuals[c] = Mat::zeros(Size(width, height), CV_32S);
        }

        // Decode each channel sequentially from the same file
        for (int c = 0; c < channels; ++c) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    residuals[c].at<int>(y, x) = decoder.decode_val();
                }
            }
        }
        return residuals;
    }

    ImageCodec(int m, int mode = 0) : m(m), mode(mode), channelsCount(0) {}
    

    /*
     * Encode a color or grayscale image and save to an encoded file
     * @param image Input image
     * @param outputFilename Output file name
     */
    void encode(const Mat &image, const string &outputFilename) {
        channelsCount = image.channels();
        vector<Mat> channels;
        split(image, channels);

        // Calculate residuals for all channels
        vector<Mat> residuals(channelsCount);
        for (int i = 0; i < channelsCount; ++i) {
            residuals[i] = calculateResiduals(channels[i]);
        }

        // Get the directory of the output file
        fs::path outputPath(outputFilename);
        fs::path outputDir = outputPath.parent_path();
        string baseFilename = outputDir / outputPath.stem().string();

        // Calculate optimal m for each channel
        vector<int> optimalMs(channelsCount);
        for (int i = 0; i < channelsCount; ++i) {
            optimalMs[i] = estimateOptimalM(residuals[i]);
        }
        
        // Save metadata including optimal m values
        string metaFilePath = baseFilename + "_meta.txt";
        ofstream metaFile(metaFilePath);
        metaFile << image.rows << " " << image.cols << " " << channelsCount << endl;
        for (int m : optimalMs) {
            metaFile << m << " ";
        }
        metaFile.close();
        
        // Encode each channel with its optimal m
        for (int i = 0; i < channelsCount; ++i) {
            string binFilePath = baseFilename + "_" + to_string(i) + ".bin";
            Golomb encoder(optimalMs[i], false, binFilePath);
            encodeResiduals({residuals[i]}, binFilePath);
        }
    }

    /*
     * Decode an encoded file to reconstruct the image
     * @param baseFilename Base file name of the encoded data
     * @return Decoded image
     */
    Mat decode(const string &baseFilename) {
        // Read metadata
        string metaFilePath = baseFilename + "_meta.txt";
        ifstream metaFile(metaFilePath);
        int rows, cols, channels;
        metaFile >> rows >> cols >> channels;
        metaFile.close();

        // Decode all channels from single file
        string binFilePath = baseFilename + ".bin";
        vector<Mat> residuals = decodeResiduals(cols, rows, channels, binFilePath);
        
        vector<Mat> channelsDecoded(channels);
        for (int i = 0; i < channels; ++i) {
            channelsDecoded[i] = reconstructChannel(residuals[i]);
        }

        Mat decodedImage;
        merge(channelsDecoded, decodedImage);
        return decodedImage;
    }

    void saveCompressedImage(const Mat &image, const string &outputPath) {
        vector<Mat> channels;
        split(image, channels);

        // Get the directory and filename from the output path
        fs::path outputFilePath(outputPath);
        fs::path outputDir = outputFilePath.parent_path();
        string baseName = outputFilePath.stem().string();
        
        // Create paths for binary data and metadata
        string binPath = (outputDir / (baseName + ".bin")).string();
        string metaPath = (outputDir / (baseName + "_meta.txt")).string();

        // Calculate residuals and save them to binary file
        vector<Mat> residuals(channels.size());
        for (int i = 0; i < channels.size(); ++i) {
            residuals[i] = calculateResiduals(channels[i]);
        }
        encodeResiduals(residuals, binPath);

        // Save metadata
        ofstream metaFile(metaPath);
        metaFile << image.rows << " " << image.cols << " " << channels.size() << " " << m << endl;
        metaFile.close();

        // Decode the binary file to get the reconstructed image
        vector<Mat> decodedResiduals = decodeResiduals(image.cols, image.rows, channels.size(), binPath);
        vector<Mat> reconstructedChannels(channels.size());
        for (int i = 0; i < channels.size(); ++i) {
            reconstructedChannels[i] = reconstructChannel(decodedResiduals[i]);
        }

        // Create and save the reconstructed image
        Mat reconstructedImage;
        merge(reconstructedChannels, reconstructedImage);
        imwrite(outputPath, reconstructedImage);
    }

    // New method to analyze all predictors
    map<string, PredictorMetrics> analyzePredictors(const Mat& image) {
        map<string, PredictorMetrics> results;
        vector<Mat> channels;
        split(image, channels);
        
        vector<string> predictorNames = {"West", "North", "Northwest", "JPEG-LS"};
        
        for(int i = 0; i < channels.size(); i++) {
            for(int p = 0; p < 4; p++) {
                string key = predictorNames[p] + "_channel" + to_string(i);
                results[key] = evaluatePredictor(channels[i], p);
            }
        }
        
        return results;
    }

};