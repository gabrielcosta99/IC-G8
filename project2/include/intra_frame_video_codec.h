#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Image_codec.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class IntraFrameVideoCodec {
private:
    ImageCodec imageCodec;
    int width;
    int height;
    int frameCount;

    // Calculate frame sizes for YUV420p
    size_t getYSize() const { return width * height; }
    size_t getUVSize() const { return (width/2) * (height/2); }
    
    void validateDimensions() const {
        if (width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0) {
            throw runtime_error("Invalid dimensions for YUV420p");
        }
    }

    // Spatial prediction for a single channel
    Mat predictChannel(const Mat& channel) const {
        Mat predictions(channel.size(), CV_8UC1);
        
        for (int y = 0; y < channel.rows; ++y) {
            // First pixel in row uses default prediction (middle value)
            predictions.at<uchar>(y, 0) = 128;
            
            // Rest of the pixels use left neighbor as predictor
            for (int x = 1; x < channel.cols; ++x) {
                predictions.at<uchar>(y, x) = channel.at<uchar>(y, x-1);
            }
        }
        return predictions;
    }

    // Calculate residuals with prediction
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

    // Reconstruct channel from residuals using prediction
    Mat reconstructChannelWithPrediction(const Mat& residuals) const {
        Mat result(residuals.size(), CV_8UC1);
        
        for (int y = 0; y < residuals.rows; ++y) {
            // First pixel in row uses default prediction
            int predicted = 128;
            int pixel = predicted + residuals.at<int>(y, 0);
            result.at<uchar>(y, 0) = saturate_cast<uchar>(pixel);
            
            // Rest of the pixels use left neighbor as predictor
            for (int x = 1; x < residuals.cols; ++x) {
                predicted = result.at<uchar>(y, x-1);
                pixel = predicted + residuals.at<int>(y, x);
                result.at<uchar>(y, x) = saturate_cast<uchar>(pixel);
            }
        }
        return result;
    }

    // Read a single YUV420p frame
    vector<Mat> readYUV420Frame(ifstream& file) {
        vector<Mat> planes;
        
        // Y plane (full resolution)
        Mat Y(height, width, CV_8UC1);
        if (!file.read(reinterpret_cast<char*>(Y.data), getYSize())) {
            throw runtime_error("Failed to read Y plane");
        }
        planes.push_back(Y);
        
        // U plane (quarter resolution)
        Mat U(height/2, width/2, CV_8UC1);
        if (!file.read(reinterpret_cast<char*>(U.data), getUVSize())) {
            throw runtime_error("Failed to read U plane");
        }
        planes.push_back(U);
        
        // V plane (quarter resolution)
        Mat V(height/2, width/2, CV_8UC1);
        if (!file.read(reinterpret_cast<char*>(V.data), getUVSize())) {
            throw runtime_error("Failed to read V plane");
        }
        planes.push_back(V);
        
        return planes;
    }

    // Write a single YUV420p frame
    void writeYUV420Frame(const vector<Mat>& planes, ofstream& file) {
        if (planes.size() != 3) {
            throw runtime_error("Invalid number of planes");
        }

        // Write Y plane
        if (!file.write(reinterpret_cast<const char*>(planes[0].data), getYSize())) {
            throw runtime_error("Failed to write Y plane");
        }
        
        // Write U plane
        if (!file.write(reinterpret_cast<const char*>(planes[1].data), getUVSize())) {
            throw runtime_error("Failed to write U plane");
        }
        
        // Write V plane
        if (!file.write(reinterpret_cast<const char*>(planes[2].data), getUVSize())) {
            throw runtime_error("Failed to write V plane");
        }
    }

public:
    IntraFrameVideoCodec(int m, int width, int height, int frameCount) 
        : imageCodec(m), width(width), height(height), frameCount(frameCount) {
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

        // Create output file
        ofstream output(outputPath + ".bin", ios::binary);
        if (!output) {
            throw runtime_error("Could not create output file");
        }

        cout << "Encoding " << frameCount << " frames..." << endl;
        
        for (int f = 0; f < frameCount; ++f) {
            // Read YUV420p frame
            vector<Mat> planes = readYUV420Frame(input);
            
            // Process Y plane (full resolution) with prediction
            Mat yResiduals = calculateResidualsWithPrediction(planes[0]);
            output.write(reinterpret_cast<char*>(yResiduals.data), height * width * sizeof(int));
            
            // Process U plane (quarter resolution) with prediction
            Mat uResiduals = calculateResidualsWithPrediction(planes[1]);
            output.write(reinterpret_cast<char*>(uResiduals.data), getUVSize() * sizeof(int));
            
            // Process V plane (quarter resolution) with prediction
            Mat vResiduals = calculateResidualsWithPrediction(planes[2]);
            output.write(reinterpret_cast<char*>(vResiduals.data), getUVSize() * sizeof(int));
            
            if (f % 10 == 0) {
                cout << "Encoded frame " << f << "/" << frameCount << endl;
            }
        }
        
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

        // Open input and output files
        ifstream input(inputPath + ".bin", ios::binary);
        ofstream output(outputPath, ios::binary);
        if (!input || !output) {
            throw runtime_error("Could not open input/output files");
        }

        cout << "Decoding " << frameCount << " frames..." << endl;

        for (int f = 0; f < frameCount; ++f) {
            vector<Mat> reconstructedPlanes;
            
            // Reconstruct Y plane with prediction
            Mat yResiduals(height, width, CV_32SC1);
            input.read(reinterpret_cast<char*>(yResiduals.data), height * width * sizeof(int));
            reconstructedPlanes.push_back(reconstructChannelWithPrediction(yResiduals));
            
            // Reconstruct U plane with prediction
            Mat uResiduals(height/2, width/2, CV_32SC1);
            input.read(reinterpret_cast<char*>(uResiduals.data), getUVSize() * sizeof(int));
            reconstructedPlanes.push_back(reconstructChannelWithPrediction(uResiduals));
            
            // Reconstruct V plane with prediction
            Mat vResiduals(height/2, width/2, CV_32SC1);
            input.read(reinterpret_cast<char*>(vResiduals.data), getUVSize() * sizeof(int));
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