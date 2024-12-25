//#ifdef IMAGE_CODEC_H
//#define IMAGE_CODEC_H

#pragma once

#include <vector>
#include <cmath> // For ceil, log2
#include <iostream>
#include "Golomb.h" // Include the Golomb class for encoding/decoding
#include <opencv2/opencv.hpp>




using namespace cv;
using namespace std;

class ImageCodec {
private:
    int m;      // Golomb parameter
    int mode;   // Prediction mode (0: A, 1: Average, etc.)
    int channelsCount; // Number of channels in the image

    /*
    * Calculate the A predictor for a single channel
    */
    int predictorA(int x, int y, const Mat &channel) {
        return (x > 0) ? channel.at<uchar>(y, x - 1) : 0;
    }
public:
    /*
    * Calculate residuals for a single channel using the A predictor
    */
    Mat calculateResiduals(const Mat &channel) {
        Mat residuals = Mat::zeros(channel.size(), CV_32S);
        for (int y = 0; y < channel.rows; ++y) {
            for (int x = 0; x < channel.cols; ++x) {
                int predicted = predictorA(x, y, channel);
                residuals.at<int>(y, x) = channel.at<uchar>(y, x) - predicted;
            }
        }
        return residuals;
    }

    /*
    * Reconstruct a single channel using residuals and the A predictor
    */
    Mat reconstructChannel(const Mat &residuals) {
        Mat channel = Mat::zeros(residuals.size(), CV_8U);
        for (int y = 0; y < residuals.rows; ++y) {
            for (int x = 0; x < residuals.cols; ++x) {
                int predicted = predictorA(x, y, channel);
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

        // Encode all channels to a single file
        encodeResiduals(residuals, outputFilename + ".bin");

        // Save metadata (image dimensions and channels count) for decoding
        ofstream metaFile(outputFilename + "_meta.txt");
        metaFile << image.rows << " " << image.cols << " " << channelsCount << endl;
        metaFile.close();
    }

    /*
     * Decode an encoded file to reconstruct the image
     * @param baseFilename Base file name of the encoded data
     * @return Decoded image
     */
    Mat decode(const string &baseFilename) {
        // Read metadata
        ifstream metaFile(baseFilename + "_meta.txt");
        int rows, cols, channels;
        metaFile >> rows >> cols >> channels;
        metaFile.close();

        // Decode all channels from single file
        vector<Mat> residuals = decodeResiduals(cols, rows, channels, baseFilename + ".bin");
        
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

        for (int i = 0; i < channels.size(); ++i) {
            Mat residuals = calculateResiduals(channels[i]);
            channels[i] = reconstructChannel(residuals); // Use the predicted + residuals
        }

        Mat compressedImage;
        merge(channels, compressedImage);

        imwrite(outputPath, compressedImage); // Save the reconstructed image
    }



};

//#endif // DEBUG