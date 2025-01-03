#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Image_codec.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

struct BlockData {
    bool useIntraMode;
    Point2i motionVector;
    Mat residuals;
};

class InterFrameVideoCodec {
private:
    ImageCodec imageCodec;
    int width;
    int height;
    int frameCount;
    int iFrameInterval;    // Interval between I-frames
    int blockSize;         // Size of blocks for motion estimation
    int searchRange;       // Search range for motion estimation
    string format;         // Video format

    // Helper functions from previous implementation
    size_t getYSize() const { return width * height; }
    size_t getUVSize() const { return (width/2) * (height/2); }
    
    void validateDimensions() const {
        if (width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0) {
            throw runtime_error("Invalid dimensions for YUV420p");
        }
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

    // Spatial prediction for a single channel
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

    // Motion estimation for a single block
    Point2i estimateMotion(const Mat& currentBlock, const Mat& referenceFrame, 
                          const Point& blockPos) const {
        int bestX = 0, bestY = 0;
        int minSAD = INT_MAX;
        
        int startX = max(0, blockPos.x - searchRange);
        int endX = min(referenceFrame.cols - blockSize, blockPos.x + searchRange);
        int startY = max(0, blockPos.y - searchRange);
        int endY = min(referenceFrame.rows - blockSize, blockPos.y + searchRange);
        
        for (int y = startY; y <= endY; ++y) {
            for (int x = startX; x <= endX; ++x) {
                Mat candidateBlock = referenceFrame(
                    Rect(x, y, blockSize, blockSize));
                
                int SAD = 0;
                for (int by = 0; by < blockSize; ++by) {
                    for (int bx = 0; bx < blockSize; ++bx) {
                        SAD += abs(currentBlock.at<uchar>(by, bx) - 
                                 candidateBlock.at<uchar>(by, bx));
                    }
                }
                
                if (SAD < minSAD) {
                    minSAD = SAD;
                    bestX = x - blockPos.x;
                    bestY = y - blockPos.y;
                }
            }
        }
        
        return Point2i(bestX, bestY);
    }

    int estimateBlockBits(const Mat& residuals, const Point2i& mv = Point2i(0,0)) const {
        // Simple estimation: sum of absolute values + motion vector overhead
        int bits = 0;
        for(int y = 0; y < residuals.rows; y++) {
            for(int x = 0; x < residuals.cols; x++) {
                bits += abs(residuals.at<int>(y, x));
            }
        }
        
        // Add overhead for motion vector (if present)
        if(mv.x != 0 || mv.y != 0) {
            bits += (abs(mv.x) + abs(mv.y)) * 8; // Assume 8 bits per component
        }
        
        return bits;
    }

    // Perform intra prediction for a block
    Mat predictBlock(const Mat& frame, const Rect& blockRect) const {
        Mat block = frame(blockRect);
        Mat predictions(block.size(), CV_8UC1);
        
        // Use left neighbor for prediction when available
        if(blockRect.x > 0) {
            for(int y = 0; y < block.rows; y++) {
                uchar leftPixel = frame.at<uchar>(blockRect.y + y, blockRect.x - 1);
                for(int x = 0; x < block.cols; x++) {
                    predictions.at<uchar>(y, x) = leftPixel;
                }
            }
        } else {
            predictions = Scalar(128); // Default prediction
        }
        
        return predictions;
    }

    // Determine best encoding mode for a block
    BlockData determineBlockMode(const Mat& currentFrame, const Mat& referenceFrame,
                               const Point& blockPos, int currentBlockSize) const {
        BlockData result;
        Rect blockRect(blockPos.x, blockPos.y, 
                      min(currentBlockSize, currentFrame.cols - blockPos.x),
                      min(currentBlockSize, currentFrame.rows - blockPos.y));
        Mat currentBlock = currentFrame(blockRect);
        
        // Try inter-frame coding
        Point2i mv = estimateMotion(currentBlock, referenceFrame, blockPos);
        Mat interPrediction = referenceFrame(
            Rect(blockPos.x + mv.x, blockPos.y + mv.y, blockRect.width, blockRect.height));
        Mat interResiduals;
        subtract(currentBlock, interPrediction, interResiduals, noArray(), CV_32SC1);
        int interBits = estimateBlockBits(interResiduals, mv);
        
        // Try intra-frame coding
        Mat intraPrediction = predictBlock(currentFrame, blockRect);
        Mat intraResiduals;
        subtract(currentBlock, intraPrediction, intraResiduals, noArray(), CV_32SC1);
        int intraBits = estimateBlockBits(intraResiduals);
        
        // Choose the mode with lower bit cost
        if(intraBits < interBits) {
            result.useIntraMode = true;
            result.motionVector = Point2i(0, 0);
            result.residuals = intraResiduals;
        } else {
            result.useIntraMode = false;
            result.motionVector = mv;
            result.residuals = interResiduals;
        }
        
        return result;
    }

    // Modified encodePFrame method to use mode decision
    void encodePFrame(const Mat& currentFrame, const Mat& referenceFrame, 
                     vector<Point2i>& motionVectors, Mat& residuals,
                     vector<bool>& blockModes, int currentBlockSize) const {
        motionVectors.clear();
        blockModes.clear();
        residuals = Mat::zeros(currentFrame.size(), CV_32SC1);
        
        for(int y = 0; y < currentFrame.rows; y += currentBlockSize) {
            for(int x = 0; x < currentFrame.cols; x += currentBlockSize) {
                BlockData blockData = determineBlockMode(currentFrame, referenceFrame,
                                                       Point(x, y), currentBlockSize);
                
                // Store mode decision and block data
                blockModes.push_back(blockData.useIntraMode);
                motionVectors.push_back(blockData.motionVector);
                
                // Copy residuals to output
                Rect blockRect(x, y, 
                             min(currentBlockSize, currentFrame.cols - x),
                             min(currentBlockSize, currentFrame.rows - y));
                blockData.residuals.copyTo(residuals(blockRect));
            }
        }
    }

    void writeBlockModes(const vector<bool>& blockModes, ofstream& output) const {
        // Write size first
        size_t size = blockModes.size();
        output.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
        
        // Write each bool as a byte
        for (bool mode : blockModes) {
            char value = mode ? 1 : 0;
            output.write(&value, 1);
        }
    }

    // Fix decode function to handle vector<bool> properly
    void readBlockModes(vector<bool>& blockModes, ifstream& input) const {
        // Read size first
        size_t size;
        input.read(reinterpret_cast<char*>(&size), sizeof(size_t));
        
        // Read each bool as a byte
        blockModes.clear();
        blockModes.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            char value;
            input.read(&value, 1);
            blockModes.push_back(value != 0);
        }
    }

        void writeResidualsGolomb(const Mat& residuals, Golomb& golomb) const {
        for(int y = 0; y < residuals.rows; y++) {
            for(int x = 0; x < residuals.cols; x++) {
                golomb.encode(residuals.at<int>(y, x));
            }
        }
    }

    void readResidualsGolomb(Mat& residuals, Golomb& golomb) const {
        for(int y = 0; y < residuals.rows; y++) {
            for(int x = 0; x < residuals.cols; x++) {
                residuals.at<int>(y, x) = golomb.decode_val();
            }
        }
    }

    void writeMotionVectorsGolomb(const vector<Point2i>& motionVectors, Golomb& golomb) const {
        for(const auto& mv : motionVectors) {
            golomb.encode(mv.x);
            golomb.encode(mv.y);
        }
    }

    vector<Point2i> readMotionVectorsGolomb(size_t count, Golomb& golomb) const {
        vector<Point2i> motionVectors;
        motionVectors.reserve(count);
        for(size_t i = 0; i < count; i++) {
            int x = golomb.decode_val();
            int y = golomb.decode_val();
            motionVectors.push_back(Point2i(x, y));
        }
        return motionVectors;
    }

    Mat decodePFrame(const Mat& referenceFrame, const vector<Point2i>& motionVectors,
                const vector<bool>& blockModes, const Mat& residuals, int currentBlockSize) const {
        Mat reconstructed = Mat::zeros(referenceFrame.size(), referenceFrame.type());
        int blockIdx = 0;
        
        for (int y = 0; y < reconstructed.rows; y += currentBlockSize) {
            for (int x = 0; x < reconstructed.cols; x += currentBlockSize) {
                int bw = min(currentBlockSize, reconstructed.cols - x);
                int bh = min(currentBlockSize, reconstructed.rows - y);
                Rect blockRect(x, y, bw, bh);
                Mat blockResiduals = residuals(blockRect);
                
                if (blockModes[blockIdx]) {
                    // Intra mode
                    Mat prediction = predictBlock(reconstructed, blockRect);
                    Mat reconstructedBlock(blockRect.size(), reconstructed.type());
                    
                    for(int i = 0; i < blockResiduals.rows; i++) {
                        for(int j = 0; j < blockResiduals.cols; j++) {
                            reconstructedBlock.at<uchar>(i,j) = saturate_cast<uchar>(
                                prediction.at<uchar>(i,j) + 
                                blockResiduals.at<int>(i,j)
                            );
                        }
                    }
                    reconstructedBlock.copyTo(reconstructed(blockRect));
                } else {
                    // Inter mode
                    Point2i mv = motionVectors[blockIdx];
                    Rect predRect(x + mv.x, y + mv.y, bw, bh);
                    predRect = predRect & Rect(0, 0, referenceFrame.cols, referenceFrame.rows);
                    Mat predBlock = referenceFrame(predRect);
                    
                    Mat reconstructedBlock(blockRect.size(), reconstructed.type());
                    for(int i = 0; i < blockResiduals.rows; i++) {
                        for(int j = 0; j < blockResiduals.cols; j++) {
                            reconstructedBlock.at<uchar>(i,j) = saturate_cast<uchar>(
                                predBlock.at<uchar>(i,j) + 
                                blockResiduals.at<int>(i,j)
                            );
                        }
                    }
                    reconstructedBlock.copyTo(reconstructed(blockRect));
                }
                blockIdx++;
            }
        }
        return reconstructed;
    }

public:
    InterFrameVideoCodec(int m, int width, int height, int frameCount,
                        int iFrameInterval, int blockSize, int searchRange, 
                        string format)
        : imageCodec(m), width(width), height(height), frameCount(frameCount),
          iFrameInterval(iFrameInterval), blockSize(blockSize), 
          searchRange(searchRange), format(format) {
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
        meta << width << " " << height << " " << frameCount << " "
             << iFrameInterval << " " << blockSize << " " << searchRange << " "
             << format << endl;
        meta.close();

        // Create Golomb encoder using the m parameter from imageCodec
        Golomb golomb(imageCodec.getM(), false, outputPath + ".bin", 1);

        Mat previousFrame;
        vector<Mat> previousPlanes;
        
        for (int f = 0; f < frameCount; ++f) {
            vector<Mat> planes = readYUV420Frame(input);
            bool isIFrame = (f % iFrameInterval == 0);
            golomb.encode(isIFrame ? 1 : 0);
            
            if (isIFrame) {
                for (int i = 0; i < 3; ++i) {
                    Mat residuals = calculateResidualsWithPrediction(planes[i]);
                    writeResidualsGolomb(residuals, golomb);
                }
                previousPlanes = planes;
            } else {
                for (int i = 0; i < 3; ++i) {
                    vector<Point2i> motionVectors;
                    vector<bool> blockModes;
                    Mat residuals;
                    
                    int channelBlockSize = (i == 0) ? blockSize : blockSize/2;
                    
                    encodePFrame(planes[i], previousPlanes[i], motionVectors, 
                               residuals, blockModes, channelBlockSize);
                    
                    // Write size and data using Golomb coding
                    golomb.encode(motionVectors.size());
                    writeMotionVectorsGolomb(motionVectors, golomb);
                    
                    // Write block modes
                    for(bool mode : blockModes) {
                        golomb.encode(mode ? 1 : 0);
                    }
                    
                    writeResidualsGolomb(residuals, golomb);
                }
                previousPlanes = planes;
            }
            
            if (f % 10 == 0) {
                cout << "Encoded frame " << f << "/" << frameCount << endl;
            }
        }
        
        golomb.end();
        cout << "Encoding complete" << endl;
    }

    void decode(const string& inputPath, const string& outputPath) {
        ifstream meta(inputPath + ".meta");
        if (!meta) {
            throw runtime_error("Could not open metadata file");
        }
        meta >> width >> height >> frameCount >> iFrameInterval >> blockSize 
             >> searchRange >> format;
        meta.close();
        validateDimensions();

        // Create Golomb decoder using the m parameter from imageCodec
        Golomb golomb(imageCodec.getM(), true, inputPath + ".bin", 1);
        ofstream output(outputPath, ios::binary);
        if (!output) {
            throw runtime_error("Could not open output file");
        }

        vector<Mat> previousPlanes;
        for (int f = 0; f < frameCount; ++f) {
            bool isIFrame = golomb.decode_val() == 1;
            vector<Mat> reconstructedPlanes;
            
            if (isIFrame) {
                for (int i = 0; i < 3; ++i) {
                    Mat residuals(i == 0 ? height : height/2,
                                i == 0 ? width : width/2, CV_32SC1);
                    readResidualsGolomb(residuals, golomb);
                    reconstructedPlanes.push_back(
                        reconstructChannelWithPrediction(residuals));
                }
            } else {
                for (int i = 0; i < 3; ++i) {
                    size_t numVectors = golomb.decode_val();
                    vector<Point2i> motionVectors = 
                        readMotionVectorsGolomb(numVectors, golomb);
                    
                    vector<bool> blockModes;
                    for(size_t j = 0; j < numVectors; j++) {
                        blockModes.push_back(golomb.decode_val() == 1);
                    }
                    
                    int channelWidth = (i == 0) ? width : width/2;
                    int channelHeight = (i == 0) ? height : height/2;
                    int channelBlockSize = (i == 0) ? blockSize : blockSize/2;
                    
                    Mat residuals(channelHeight, channelWidth, CV_32SC1);
                    readResidualsGolomb(residuals, golomb);
                    
                    Mat reconstructedChannel = decodePFrame(previousPlanes[i], 
                                                          motionVectors,
                                                          blockModes, 
                                                          residuals,
                                                          channelBlockSize);
                    reconstructedPlanes.push_back(reconstructedChannel);
                }
            }
            
            writeYUV420Frame(reconstructedPlanes, output);
            previousPlanes = reconstructedPlanes;
            
            if (f % 10 == 0) {
                cout << "Decoded frame " << f << "/" << frameCount << endl;
            }
        }
        
        cout << "Decoding complete" << endl;
    }
};