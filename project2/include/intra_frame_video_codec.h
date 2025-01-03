#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Image_codec.h"
#include "Golomb.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class IntraFrameVideoCodec {
private:
    ImageCodec imageCodec;
    int width;
    int height;
    int frameCount;
    int m;  // Golomb parameter

    // Calculate frame sizes for YUV420p
    size_t getYSize() const { return width * height; }
    size_t getUVSize() const { return (width/2) * (height/2); }
    
    void validateDimensions() const {
        if (width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0) {
            throw runtime_error("Invalid dimensions for YUV420p");
        }
    }

    // Spatial prediction for a single channel (unchanged)
    Mat predictChannel(const Mat& channel) const {
        Mat predictions(channel.size(), CV_8UC1);
        
        for (int y = 0; y < channel.rows; ++y) {
            predictions.at<uchar>(y, 0) = 128;
            for (int x = 1; x < channel.cols; ++x) {
                predictions.at<uchar>(y, x) = channel.at<uchar>(y, x-1);
            }
        }
        return predictions;
    }

    // Calculate residuals with prediction (unchanged)
    Mat calculateResidualsWithPrediction(const Mat& channel) const {
        Mat predictions = predictChannel(channel);
        Mat residuals(channel.size(), CV_32SC1);
        
        for (int y = 0; y < channel.rows; ++y) {
            for (int x = 0; x < channel.cols; ++x) {
                residuals.at<int>(y, x) = static_cast<int>(channel.at<uchar>(y, x)) - 
                                        static_cast<int>(predictions.at<uchar>(y, x));
            }
        }
        return residuals;
    }

    // Write residuals using Golomb coding
    void writeResiduals(const Mat& residuals, Golomb& golomb) {
        for (int y = 0; y < residuals.rows; ++y) {
            for (int x = 0; x < residuals.cols; ++x) {
                golomb.encode(residuals.at<int>(y, x));
            }
        }
    }

    // Read residuals using Golomb coding
    Mat readResiduals(Golomb& golomb, int rows, int cols) {
        Mat residuals(rows, cols, CV_32SC1);
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                residuals.at<int>(y, x) = golomb.decode_val();
            }
        }
        return residuals;
    }

    // Reconstruct channel from residuals (unchanged)
    Mat reconstructChannelWithPrediction(const Mat& residuals) const {
        Mat result(residuals.size(), CV_8UC1);
        
        for (int y = 0; y < residuals.rows; ++y) {
            int predicted = 128;
            int pixel = predicted + residuals.at<int>(y, 0);
            result.at<uchar>(y, 0) = saturate_cast<uchar>(pixel);
            
            for (int x = 1; x < residuals.cols; ++x) {
                predicted = result.at<uchar>(y, x-1);
                pixel = predicted + residuals.at<int>(y, x);
                result.at<uchar>(y, x) = saturate_cast<uchar>(pixel);
            }
        }
        return result;
    }

    // Read/Write YUV420p frame methods (unchanged)
    vector<Mat> readYUV420Frame(ifstream& file) {
        vector<Mat> planes;
        
        Mat Y(height, width, CV_8UC1);
        if (!file.read(reinterpret_cast<char*>(Y.data), getYSize())) {
            throw runtime_error("Failed to read Y plane");
        }
        planes.push_back(Y);
        
        Mat U(height/2, width/2, CV_8UC1);
        if (!file.read(reinterpret_cast<char*>(U.data), getUVSize())) {
            throw runtime_error("Failed to read U plane");
        }
        planes.push_back(U);
        
        Mat V(height/2, width/2, CV_8UC1);
        if (!file.read(reinterpret_cast<char*>(V.data), getUVSize())) {
            throw runtime_error("Failed to read V plane");
        }
        planes.push_back(V);
        
        return planes;
    }

    void writeYUV420Frame(const vector<Mat>& planes, ofstream& file) {
        if (planes.size() != 3) {
            throw runtime_error("Invalid number of planes");
        }

        if (!file.write(reinterpret_cast<const char*>(planes[0].data), getYSize())) {
            throw runtime_error("Failed to write Y plane");
        }
        
        if (!file.write(reinterpret_cast<const char*>(planes[1].data), getUVSize())) {
            throw runtime_error("Failed to write U plane");
        }
        
        if (!file.write(reinterpret_cast<const char*>(planes[2].data), getUVSize())) {
            throw runtime_error("Failed to write V plane");
        }
    }

public:
    IntraFrameVideoCodec(int m, int width, int height, int frameCount) 
        : imageCodec(m), width(width), height(height), frameCount(frameCount), m(m) {
        validateDimensions();
    }

    void encode(const string& inputPath, const string& outputPath) {
        ifstream input(inputPath, ios::binary);
        if (!input) {
            throw runtime_error("Could not open input file: " + inputPath);
        }

        // Write metadata
        ofstream meta(outputPath + ".meta");
        if (!meta) {
            throw runtime_error("Could not create metadata file");
        }
        meta << width << " " << height << " " << frameCount << endl;
        meta.close();

        // Create Golomb encoder
        Golomb golomb(m, false, outputPath + ".bin", 1);  // Using zigzag mode

        cout << "Encoding " << frameCount << " frames..." << endl;
        
        for (int f = 0; f < frameCount; ++f) {
            vector<Mat> planes = readYUV420Frame(input);
            
            // Process and encode each plane
            Mat yResiduals = calculateResidualsWithPrediction(planes[0]);
            writeResiduals(yResiduals, golomb);
            
            Mat uResiduals = calculateResidualsWithPrediction(planes[1]);
            writeResiduals(uResiduals, golomb);
            
            Mat vResiduals = calculateResidualsWithPrediction(planes[2]);
            writeResiduals(vResiduals, golomb);
            
            if (f % 10 == 0) {
                cout << "Encoded frame " << f << "/" << frameCount << endl;
            }
        }
        
        golomb.end();
        cout << "Encoding complete" << endl;
    }

    void decode(const string& inputPath, const string& outputPath) {
        // Read metadata
        ifstream meta(inputPath + ".meta");
        if (!meta) {
            throw runtime_error("Could not open metadata file");
        }
        meta >> width >> height >> frameCount;
        meta.close();
        validateDimensions();

        // Create Golomb decoder
        Golomb golomb(m, true, inputPath + ".bin", 1);  // Using zigzag mode

        // Open output file
        ofstream output(outputPath, ios::binary);
        if (!output) {
            throw runtime_error("Could not open output file");
        }

        cout << "Decoding " << frameCount << " frames..." << endl;

        for (int f = 0; f < frameCount; ++f) {
            vector<Mat> reconstructedPlanes;
            
            // Read and reconstruct Y plane
            Mat yResiduals = readResiduals(golomb, height, width);
            reconstructedPlanes.push_back(reconstructChannelWithPrediction(yResiduals));
            
            // Read and reconstruct U plane
            Mat uResiduals = readResiduals(golomb, height/2, width/2);
            reconstructedPlanes.push_back(reconstructChannelWithPrediction(uResiduals));
            
            // Read and reconstruct V plane
            Mat vResiduals = readResiduals(golomb, height/2, width/2);
            reconstructedPlanes.push_back(reconstructChannelWithPrediction(vResiduals));
            
            // Write reconstructed YUV420p frame
            writeYUV420Frame(reconstructedPlanes, output);
            
            if (f % 10 == 0) {
                cout << "Decoded frame " << f << "/" << frameCount << endl;
            }
        }
        
        cout << "Decoding complete" << endl;
    }
};