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
    bool skipMode;       // Used for near-zero residual blocks
    int predictionMode;  // Multiple intra prediction modes
};

class InterFrameVideoCodec {
protected:
    ImageCodec imageCodec;
    int width;
    int height;
    int frameCount;
    int iFrameInterval;    
    int blockSize;         
    int searchRange;       
    string y4mHeader;
    int sourceFormat;

    // Add new member variables for improved motion estimation
    const int EARLY_EXIT_THRESHOLD = 256;

    // New constants for improved compression
    const int SKIP_THRESHOLD = 3;      // Maximum absolute sum for skip mode
    const int SEARCH_STEP = 2;         // Step size for fast motion search
    const int MAX_ZERO_RUN = 1024;     // Maximum zero run length
    const float LAMBDA = 0.9;          // Rate-distortion trade-off factor

    // Helper functions from previous implementation
    size_t getYSize() const { return width * height; }
    size_t getUVSize() const { return (width/2) * (height/2); }
    size_t getSourceUVSize() const {
        switch(sourceFormat) {
            case 420: return (width/2) * (height/2);
            case 422: return (width/2) * height;
            case 444: return width * height;
            default: throw runtime_error("Unsupported format");
        }
    }
    
    void validateDimensions() const {
        if (width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0) {
            throw runtime_error("Invalid dimensions for YUV420p");
        }
    }

    int parseFormat(const string& header) {
        size_t c_pos = header.find("C");
        if (c_pos == string::npos) {
            return 420; // Default format if not specified
        }

        string formatStr = header.substr(c_pos, header.find(' ', c_pos) - c_pos);
        if (formatStr.find("420") != string::npos) return 420;
        if (formatStr.find("422") != string::npos) return 422;
        if (formatStr.find("444") != string::npos) return 444;
        
        throw runtime_error("Unsupported YUV format in Y4M header");
    }

    bool parseY4MHeader(istream& file) {
        string header;
        getline(file, header);
        
        if (header.substr(0, 10) != "YUV4MPEG2 ") {
            return false;
        }

        sourceFormat = parseFormat(header);
        
        // Modify header to always indicate 420 format
        size_t c_pos = header.find("C");
        if (c_pos != string::npos) {
            size_t space_pos = header.find(' ', c_pos);
            header = header.substr(0, c_pos) + "C420" + 
                    (space_pos != string::npos ? header.substr(space_pos) : "");
        }

        y4mHeader = header + "\n";
        
        size_t w_pos = header.find("W");
        size_t h_pos = header.find("H");
        
        if (w_pos == string::npos || h_pos == string::npos) {
            return false;
        }

        string w_str = header.substr(w_pos + 1, header.find(' ', w_pos) - w_pos - 1);
        string h_str = header.substr(h_pos + 1, header.find(' ', h_pos) - h_pos - 1);
        
        width = stoi(w_str);
        height = stoi(h_str);
        
        return true;
    }

    Mat convertUVto420(const Mat& uv, int srcFormat) {
        Mat result;
        switch(srcFormat) {
            case 420:
                return uv.clone();
            case 422:
                resize(uv, result, Size(uv.cols, uv.rows/2), 0, 0, INTER_AREA);
                return result;
            case 444:
                resize(uv, result, Size(uv.cols/2, uv.rows/2), 0, 0, INTER_AREA);
                return result;
            default:
                throw runtime_error("Unsupported format conversion");
        }
    }

    vector<Mat> readY4MFrame(ifstream& file) {
        string frameHeader;
        getline(file, frameHeader);
        
        if (frameHeader != "FRAME") {
            throw runtime_error("Invalid frame marker");
        }

        vector<Mat> planes;
        
        // Read Y plane (same for all formats)
        Mat Y(height, width, CV_8UC1);
        file.read(reinterpret_cast<char*>(Y.data), width * height);
        planes.push_back(Y);

        // Read U and V planes based on source format
        size_t uvWidth = (sourceFormat == 444) ? width : width/2;
        size_t uvHeight = (sourceFormat == 420) ? height/2 : height;
        
        Mat U(uvHeight, uvWidth, CV_8UC1);
        Mat V(uvHeight, uvWidth, CV_8UC1);
        
        file.read(reinterpret_cast<char*>(U.data), uvWidth * uvHeight);
        file.read(reinterpret_cast<char*>(V.data), uvWidth * uvHeight);
        
        // Convert to 420 if needed
        if (sourceFormat != 420) {
            planes.push_back(convertUVto420(U, sourceFormat));
            planes.push_back(convertUVto420(V, sourceFormat));
        } else {
            planes.push_back(U);
            planes.push_back(V);
        }
        
        return planes;
    }

    void writeY4MFrame(const vector<Mat>& planes, ofstream& file) {
        if (planes.size() != 3) {
            throw runtime_error("Invalid number of planes");
        }

        file << "FRAME\n";

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
        
        for (int y = 0; y < residuals.rows; y++) {
            int predicted = 128;
            int pixel = predicted + residuals.at<int>(y, 0);
            result.at<uchar>(y, 0) = saturate_cast<uchar>(pixel);
            
            for (int x = 1; x < residuals.cols; x++) {
                predicted = result.at<uchar>(y, x-1);
                pixel = predicted + residuals.at<int>(y, x);
                result.at<uchar>(y, x) = saturate_cast<uchar>(pixel);
            }
        }
        return result;
    }

    // Helper function to validate motion vector bounds
    bool isValidMotionVector(const Point2i& mv, const Point& blockPos, const Mat& reference, int currentBlockSize) const {
        Point2i newPos(blockPos.x + mv.x, blockPos.y + mv.y);
        return newPos.x >= 0 && 
               newPos.y >= 0 && 
               newPos.x + currentBlockSize <= reference.cols &&
               newPos.y + currentBlockSize <= reference.rows;
    }

    // Helper function to calculate Sum of Absolute Differences
    int calculateSAD(const Mat& currentBlock, const Mat& referenceFrame, 
                    const Point& blockPos, const Point2i& mv, int currentBlockSize) const {
        if (!isValidMotionVector(mv, blockPos, referenceFrame, currentBlockSize)) {
            return INT_MAX;
        }
        
        Rect candidateRect(blockPos.x + mv.x, blockPos.y + mv.y, 
                          currentBlock.cols, currentBlock.rows);
        Mat candidateBlock = referenceFrame(candidateRect);
        
        int SAD = 0;
        for(int y = 0; y < currentBlock.rows; y++) {
            for(int x = 0; x < currentBlock.cols; x++) {
                SAD += abs(currentBlock.at<uchar>(y, x) - candidateBlock.at<uchar>(y, x));
            }
        }
        return SAD;
    }

    Point2i estimateMotion(const Mat& currentBlock, const Mat& referenceFrame, 
                          const Point& blockPos, int currentBlockSize) const {
        // Simple zero motion vector if block or boundaries are invalid
        if (blockPos.x < 0 || blockPos.y < 0 || 
            blockPos.x + currentBlockSize > referenceFrame.cols ||
            blockPos.y + currentBlockSize > referenceFrame.rows) {
            return Point2i(0, 0);
        }

        Point2i bestMV(0, 0);
        int minSAD = INT_MAX;

        // Constrained search range to avoid boundary issues
        int xStart = max(-searchRange, -blockPos.x);
        int yStart = max(-searchRange, -blockPos.y);
        int xEnd = min(searchRange, referenceFrame.cols - blockPos.x - currentBlockSize);
        int yEnd = min(searchRange, referenceFrame.rows - blockPos.y - currentBlockSize);

        for(int dy = yStart; dy <= yEnd; dy++) {
            for(int dx = xStart; dx <= xEnd; dx++) {
                Point2i mv(dx, dy);
                Rect searchRect(blockPos.x + dx, blockPos.y + dy, currentBlockSize, currentBlockSize);
                Mat candidateBlock = referenceFrame(searchRect);
                
                int sad = 0;
                for(int y = 0; y < currentBlockSize; y++) {
                    for(int x = 0; x < currentBlockSize; x++) {
                        sad += abs(currentBlock.at<uchar>(y,x) - candidateBlock.at<uchar>(y,x));
                    }
                }
                
                if(sad < minSAD) {
                    minSAD = sad;
                    bestMV = mv;
                }
            }
        }
        return bestMV;
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

    // Remove skip mode and early termination which were causing quality issues
    BlockData determineBlockMode(const Mat& currentFrame, const Mat& referenceFrame,
                               const Point& blockPos, int currentBlockSize) const {
        BlockData result;
        Rect blockRect(blockPos.x, blockPos.y, 
                      min(currentBlockSize, currentFrame.cols - blockPos.x),
                      min(currentBlockSize, currentFrame.rows - blockPos.y));
        
        // Ensure block is within frame boundaries
        if (blockRect.x < 0 || blockRect.y < 0 || 
            blockRect.x + blockRect.width > currentFrame.cols ||
            blockRect.y + blockRect.height > currentFrame.rows) {
            throw runtime_error("Invalid block boundaries");
        }
        
        Mat currentBlock = currentFrame(blockRect);
        
        // Try inter-frame coding
        Point2i mv = estimateMotion(currentBlock, referenceFrame, blockPos, currentBlockSize);
        
        // Check if motion vector is valid
        if (isValidMotionVector(mv, blockPos, referenceFrame, currentBlockSize)) {
            Rect predRect(blockPos.x + mv.x, blockPos.y + mv.y, 
                         blockRect.width, blockRect.height);
            Mat interPrediction = referenceFrame(predRect);
            Mat interResiduals;
            subtract(currentBlock, interPrediction, interResiduals, noArray(), CV_32SC1);
            
            result.useIntraMode = false;
            result.motionVector = mv;
            result.residuals = interResiduals;
        } else {
            // Fallback to intra mode if motion vector is invalid
            result.useIntraMode = true;
            result.motionVector = Point2i(0, 0);
            Mat intraPrediction = predictBlock(currentFrame, blockRect);
            Mat intraResiduals;
            subtract(currentBlock, intraPrediction, intraResiduals, noArray(), CV_32SC1);
            result.residuals = intraResiduals;
        }
        
        result.skipMode = false;
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

    // Fixed residual writing - remove run length encoding which was causing issues
    virtual void writeResidualsGolomb(const Mat& residuals, Golomb& golomb) const {
        // Write dimensions
        golomb.encode(residuals.rows);
        golomb.encode(residuals.cols);
        
        // Write all residual values directly
        for(int y = 0; y < residuals.rows; y++) {
            for(int x = 0; x < residuals.cols; x++) {
                golomb.encode(residuals.at<int>(y, x));
            }
        }
    }

    // Fixed residual reading to match the writing
    virtual void readResidualsGolomb(Mat& residuals, Golomb& golomb) const {
        int rows = golomb.decode_val();
        int cols = golomb.decode_val();
        residuals = Mat::zeros(rows, cols, CV_32SC1);
        
        // Read all values directly
        for(int y = 0; y < rows; y++) {
            for(int x = 0; x < cols; x++) {
                residuals.at<int>(y, x) = golomb.decode_val();
            }
        }
    }

    // Add differential encoding for motion vectors
    virtual void writeMotionVectorsGolomb(const vector<Point2i>& motionVectors, Golomb& golomb) const {
        if (motionVectors.empty()) return;
        
        // Write first vector directly
        golomb.encode(motionVectors[0].x);
        golomb.encode(motionVectors[0].y);
        
        // Write differences for subsequent vectors
        for(size_t i = 1; i < motionVectors.size(); i++) {
            golomb.encode(motionVectors[i].x - motionVectors[i-1].x);
            golomb.encode(motionVectors[i].y - motionVectors[i-1].y);
        }
    }

    virtual vector<Point2i> readMotionVectorsGolomb(size_t count, Golomb& golomb) const {
        vector<Point2i> motionVectors;
        if (count == 0) return motionVectors;
        
        // Read first vector
        Point2i prev;
        prev.x = golomb.decode_val();
        prev.y = golomb.decode_val();
        motionVectors.push_back(prev);
        
        // Read and reconstruct subsequent vectors
        for(size_t i = 1; i < count; i++) {
            Point2i current;
            current.x = prev.x + golomb.decode_val();
            current.y = prev.y + golomb.decode_val();
            motionVectors.push_back(current);
            prev = current;
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

    // New helper methods for compression
    bool isSkippableBlock(const Mat& block, const Mat& reference, const Point& pos) const {
        int sum = 0;
        for(int y = 0; y < block.rows; y++) {
            for(int x = 0; x < block.cols; x++) {
                sum += abs(block.at<uchar>(y,x) - reference.at<uchar>(pos.y+y, pos.x+x));
                if(sum > SKIP_THRESHOLD) return false;
            }
        }
        return true;
    }

public:
    InterFrameVideoCodec(int m, int width, int height, int iFrameInterval, int blockSize, int searchRange)
        : imageCodec(m), width(width), height(height), frameCount(0),
        iFrameInterval(iFrameInterval), blockSize(blockSize), 
        searchRange(searchRange) {
        validateDimensions();
    }

    void encode(const string& inputPath, const string& outputPath) {
        ifstream input(inputPath, ios::binary);
        if (!input) {
            throw runtime_error("Could not open input file: " + inputPath);
        }

        // Get input file size
        input.seekg(0, ios::end);
        size_t inputSize = input.tellg();
        input.seekg(0, ios::beg);

        // Parse Y4M header
        if (!parseY4MHeader(input)) {
            throw runtime_error("Invalid Y4M header");
        }

        // Count frames if not provided
        if (frameCount == 0) {
            streampos current = input.tellg();
            input.seekg(0, ios::end);
            size_t fileSize = static_cast<size_t>(input.tellg()) - static_cast<size_t>(current);
            input.seekg(current, ios::beg);

            size_t frameDataSize = getYSize() + 2 * getSourceUVSize();  // Actual YUV data size
            size_t frameOverhead = 6;  // "FRAME\n" marker
            size_t totalFrameSize = frameDataSize + frameOverhead;
            
            frameCount = fileSize / totalFrameSize;
            cout << "Detected " << frameCount << " frames" << endl;
        }

        // Write metadata
        ofstream meta(outputPath + ".meta");
        if (!meta) {
            throw runtime_error("Could not create metadata file");
        }
        meta << y4mHeader;
        meta << frameCount << " " << iFrameInterval << " " << blockSize << " " 
             << searchRange << endl;
        meta.close();

        // Create Golomb encoder
        Golomb golomb(imageCodec.getM(), false, outputPath + ".bin", 1);

        Mat previousFrame;
        vector<Mat> previousPlanes;
        
        for (int f = 0; f < frameCount; ++f) {
            vector<Mat> planes = readY4MFrame(input);
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
        
        // Get compressed size
        ifstream binFile(outputPath + ".bin", ios::binary | ios::ate);
        size_t compressedSize = binFile.tellg();
        
        cout << "Original size: " << inputSize << " bytes" << endl;
        cout << "Compressed size: " << compressedSize << " bytes" << endl;
        cout << "Compression ratio: " << (float)inputSize/compressedSize << ":1" << endl;
        cout << "Encoding complete" << endl;
    }

    void decode(const string& inputPath, const string& outputPath) {
        // Read metadata
        ifstream meta(inputPath + ".meta");
        if (!meta) {
            throw runtime_error("Could not open metadata file");
        }
        
        // Read Y4M header from metadata
        getline(meta, y4mHeader);
        meta >> frameCount >> iFrameInterval >> blockSize >> searchRange;
        
        // Parse dimensions from Y4M header
        stringstream headerStream(y4mHeader);
        if (!parseY4MHeader(headerStream)) {
            throw runtime_error("Invalid Y4M header in metadata");
        }
        
        meta.close();
        validateDimensions();

        // Create Golomb decoder
        Golomb golomb(imageCodec.getM(), true, inputPath + ".bin", 1);

        // Open output file and write Y4M header
        ofstream output(outputPath, ios::binary);
        if (!output) {
            throw runtime_error("Could not open output file");
        }
        output << y4mHeader;

        vector<Mat> previousPlanes;
        // Initialize with empty planes
        previousPlanes.push_back(Mat::zeros(height, width, CV_8UC1));
        previousPlanes.push_back(Mat::zeros(height/2, width/2, CV_8UC1));
        previousPlanes.push_back(Mat::zeros(height/2, width/2, CV_8UC1));

        try {
            for (int f = 0; f < frameCount; ++f) {
                bool isIFrame = golomb.decode_val() == 1;
                vector<Mat> reconstructedPlanes;

                if (isIFrame) {
                    for (int i = 0; i < 3; ++i) {
                        Mat residuals;  // Let readResidualsGolomb allocate the proper size
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
                        
                        Mat residuals;  // Let readResidualsGolomb allocate the proper size
                        readResidualsGolomb(residuals, golomb);
                        
                        Mat reconstructedChannel = decodePFrame(previousPlanes[i], 
                                                              motionVectors,
                                                              blockModes, 
                                                              residuals,
                                                              channelBlockSize);
                        reconstructedPlanes.push_back(reconstructedChannel);
                    }
                }
                
                writeY4MFrame(reconstructedPlanes, output);
                previousPlanes = reconstructedPlanes;
                
                if (f % 10 == 0) {
                    cout << "Decoded frame " << f << "/" << frameCount << endl;
                }
            }
        } catch (const exception& e) {
            cout << "Error during decoding: " << e.what() << endl;
            throw;
        }
        
        cout << "Decoding complete" << endl;
    }
};