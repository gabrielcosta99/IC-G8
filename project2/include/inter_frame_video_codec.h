#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Image_codec.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

struct MotionVector {
    int x;
    int y;
    bool isIntraMode;  // true if block is encoded in intra mode
};

class InterFrameVideoCodec {
private:
    ImageCodec imageCodec;
    int width;
    int height;
    int frameCount;
    string format;
    int iFrameInterval;    // interval between I-frames
    int blockSize;         // size of blocks for motion estimation
    int searchRange;       // search range for motion estimation
    
    // Helper struct to store motion estimation results
    struct MEResult {
        MotionVector mv;
        double sad;        // Sum of Absolute Differences
        Mat residual;      // Residual block after motion compensation
    };

    // Perform motion estimation for a block
    MEResult estimateMotion(const Mat& currentBlock, const Mat& referenceFrame, 
                           const Point& blockPos) {
        MEResult result;
        result.mv.isIntraMode = false;
        double minSAD = numeric_limits<double>::max();
        
        // Search area boundaries
        int startX = max(0, blockPos.x - searchRange);
        int endX = min(width - blockSize, blockPos.x + searchRange);
        int startY = max(0, blockPos.y - searchRange);
        int endY = min(height - blockSize, blockPos.y + searchRange);
        
        // Search for best match
        for (int y = startY; y <= endY; y++) {
            for (int x = startX; x <= endX; x++) {
                Mat candidateBlock = referenceFrame(
                    Rect(x, y, blockSize, blockSize));
                double sad = sum(abs(currentBlock - candidateBlock))[0];
                
                if (sad < minSAD) {
                    minSAD = sad;
                    result.mv.x = x - blockPos.x;
                    result.mv.y = y - blockPos.y;
                    result.sad = sad;
                    result.residual = currentBlock - candidateBlock;
                }
            }
        }
        
        return result;
    }

    // Mode decision - choose between intra and inter coding
    void modeDecision(MEResult& result, const Mat& currentBlock) {
        // Estimate bitrate for inter coding
        double interBitrate = result.sad;  // Simplified bitrate estimation
        interBitrate += 2 * 8;  // Approximate cost of motion vector
        
        // Estimate bitrate for intra coding
        double intraBitrate = sum(abs(currentBlock))[0];  // Simplified estimation
        
        // Choose mode with lower bitrate
        if (intraBitrate < interBitrate) {
            result.mv.isIntraMode = true;
            result.residual = currentBlock;
        }
    }

    // Encode motion vectors and residuals
    void encodeMVsAndResiduals(const vector<vector<MEResult>>& meResults, 
                              const string& outputPath) {
        ofstream mvFile(outputPath + "_mv.bin", ios::binary);
        
        // Write motion vectors
        for (const auto& row : meResults) {
            for (const auto& result : row) {
                mvFile.write(reinterpret_cast<const char*>(&result.mv), 
                           sizeof(MotionVector));
            }
        }
        mvFile.close();
        
        // Create residual frame from blocks
        Mat residualFrame(height, width, CV_8UC3);
        for (int y = 0; y < meResults.size(); y++) {
            for (int x = 0; x < meResults[y].size(); x++) {
                Rect blockRect(x * blockSize, y * blockSize, 
                             blockSize, blockSize);
                meResults[y][x].residual.copyTo(
                    residualFrame(blockRect));
            }
        }
        
        // Encode residual frame using existing image codec
        imageCodec.encode(residualFrame, outputPath + "_residual");
    }

    // Decode motion vectors and residuals
    Mat decodeMVsAndResiduals(const string& inputPath, const Mat& referenceFrame) {
        ifstream mvFile(inputPath + "_mv.bin", ios::binary);
        Mat reconstructedFrame = Mat::zeros(height, width, CV_8UC3);
        
        // Read and apply motion vectors
        for (int y = 0; y < height; y += blockSize) {
            for (int x = 0; x < width; x += blockSize) {
                MotionVector mv;
                mvFile.read(reinterpret_cast<char*>(&mv), sizeof(MotionVector));
                
                if (!mv.isIntraMode) {
                    // Copy block from reference frame using motion vector
                    Rect sourceRect(x + mv.x, y + mv.y, blockSize, blockSize);
                    Rect targetRect(x, y, blockSize, blockSize);
                    referenceFrame(sourceRect).copyTo(
                        reconstructedFrame(targetRect));
                }
            }
        }
        mvFile.close();
        
        // Decode and add residuals
        Mat residualFrame = imageCodec.decode(inputPath + "_residual");
        reconstructedFrame += residualFrame;
        
        return reconstructedFrame;
    }

public:
    InterFrameVideoCodec(int m, int width, int height, int frameCount, 
                        int iFrameInterval, int blockSize, int searchRange,
                        string format = "420")
        : imageCodec(m), width(width), height(height), frameCount(frameCount),
          iFrameInterval(iFrameInterval), blockSize(blockSize), 
          searchRange(searchRange), format(format) {}

    void encode(const string& inputPath, const string& outputPath) {
        ifstream input(inputPath + ".yuv", ios::binary);
        if (!input.is_open()) {
            throw runtime_error("Could not open input file: " + inputPath);
        }

        // Save metadata
        ofstream metaFile(outputPath + "_meta.txt");
        metaFile << width << " " << height << " " << frameCount << " "
                << format << " " << iFrameInterval << " " << blockSize << " "
                << searchRange << endl;
        metaFile.close();

        Mat previousFrame;
        for (int frame = 0; frame < frameCount; ++frame) {
            // Read current frame
            Mat currentFrame = readYUVFrame(input);
            
            if (frame % iFrameInterval == 0) {  // I-frame
                imageCodec.encode(currentFrame, 
                                outputPath + "_frame" + to_string(frame));
            } else {  // P-frame
                vector<vector<MEResult>> meResults;
                
                // Process each block
                for (int y = 0; y < height; y += blockSize) {
                    vector<MEResult> rowResults;
                    for (int x = 0; x < width; x += blockSize) {
                        Mat currentBlock = currentFrame(
                            Rect(x, y, blockSize, blockSize));
                        MEResult result = estimateMotion(currentBlock, 
                                                       previousFrame, 
                                                       Point(x, y));
                        modeDecision(result, currentBlock);
                        rowResults.push_back(result);
                    }
                    meResults.push_back(rowResults);
                }
                
                // Encode motion vectors and residuals
                encodeMVsAndResiduals(meResults, 
                                    outputPath + "_frame" + to_string(frame));
            }
            
            previousFrame = currentFrame.clone();
            
            if (frame % 10 == 0) {
                cout << "Encoded frame " << frame << "/" << frameCount << endl;
            }
        }
        input.close();
    }

    void decode(const string& inputPath, const string& outputPath) {
        // Read metadata
        ifstream metaFile(inputPath + "_meta.txt");
        if (!metaFile.is_open()) {
            throw runtime_error("Could not open metadata file");
        }
        metaFile >> width >> height >> frameCount >> format 
                >> iFrameInterval >> blockSize >> searchRange;
        metaFile.close();

        ofstream output(outputPath + ".yuv", ios::binary);
        if (!output.is_open()) {
            throw runtime_error("Could not open output file");
        }

        Mat previousFrame;
        for (int frame = 0; frame < frameCount; ++frame) {
            Mat currentFrame;
            
            if (frame % iFrameInterval == 0) {  // I-frame
                currentFrame = imageCodec.decode(
                    inputPath + "_frame" + to_string(frame));
            } else {  // P-frame
                currentFrame = decodeMVsAndResiduals(
                    inputPath + "_frame" + to_string(frame), previousFrame);
            }
            
            writeYUVFrame(currentFrame, output);
            previousFrame = currentFrame.clone();
            
            if (frame % 10 == 0) {
                cout << "Decoded frame " << frame << "/" << frameCount << endl;
            }
        }
        output.close();
    }

private:
    // Reuse YUV frame reading/writing from original codec
    Mat readYUVFrame(ifstream& file) {
        Mat Y(height, width, CV_8UC1);
        Mat U(height/2, width/2, CV_8UC1);
        Mat V(height/2, width/2, CV_8UC1);

        file.read(reinterpret_cast<char*>(Y.data), width * height);
        file.read(reinterpret_cast<char*>(U.data), (width/2) * (height/2));
        file.read(reinterpret_cast<char*>(V.data), (width/2) * (height/2));

        resize(U, U, Y.size());
        resize(V, V, Y.size());

        vector<Mat> channels = {Y, U, V};
        Mat frame;
        merge(channels, frame);
        return frame;
    }

    void writeYUVFrame(const Mat& frame, ofstream& file) {
        vector<Mat> channels;
        split(frame, channels);

        Mat U_small, V_small;
        resize(channels[1], U_small, Size(width/2, height/2));
        resize(channels[2], V_small, Size(width/2, height/2));

        file.write(reinterpret_cast<char*>(channels[0].data), width * height);
        file.write(reinterpret_cast<char*>(U_small.data), (width/2) * (height/2));
        file.write(reinterpret_cast<char*>(V_small.data), (width/2) * (height/2));
        file.flush();
    }
};