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
    string format;
    
    // Calculate expected frame size in bytes
    size_t getFrameSize() const {
        size_t ySize = width * height;
        size_t uvSize = (width/2) * (height/2);
        return ySize + 2 * uvSize;
    }
    
    void validateFrame(const Mat& frame) const {
        if (frame.empty()) {
            throw runtime_error("Empty frame detected");
        }
        if (frame.size() != Size(width, height)) {
            throw runtime_error("Frame size mismatch");
        }
        if (frame.channels() != 3) {
            throw runtime_error("Invalid number of channels");
        }
    }

    Mat readYUVFrame(ifstream& file) {
        Mat Y(height, width, CV_8UC1);
        Mat U(height/2, width/2, CV_8UC1);
        Mat V(height/2, width/2, CV_8UC1);

        if (!file.read(reinterpret_cast<char*>(Y.data), width * height)) {
            throw runtime_error("Failed to read Y component");
        }
        if (!file.read(reinterpret_cast<char*>(U.data), (width/2) * (height/2))) {
            throw runtime_error("Failed to read U component");
        }
        if (!file.read(reinterpret_cast<char*>(V.data), (width/2) * (height/2))) {
            throw runtime_error("Failed to read V component");
        }

        resize(U, U, Y.size());
        resize(V, V, Y.size());

        vector<Mat> channels = {Y, U, V};
        Mat frame;
        merge(channels, frame);
        validateFrame(frame);
        return frame;
    }

    void writeYUVFrame(const Mat& frame, ofstream& file) {
        validateFrame(frame);
        
        vector<Mat> channels;
        split(frame, channels);

        Mat U_small, V_small;
        resize(channels[1], U_small, Size(width/2, height/2));
        resize(channels[2], V_small, Size(width/2, height/2));

        if (!file.write(reinterpret_cast<char*>(channels[0].data), width * height)) {
            throw runtime_error("Failed to write Y component");
        }
        if (!file.write(reinterpret_cast<char*>(U_small.data), (width/2) * (height/2))) {
            throw runtime_error("Failed to write U component");
        }
        if (!file.write(reinterpret_cast<char*>(V_small.data), (width/2) * (height/2))) {
            throw runtime_error("Failed to write V component");
        }
        file.flush();
    }

public:
    IntraFrameVideoCodec(int m, int width, int height, int frameCount, string format = "420")
        : imageCodec(m), width(width), height(height), frameCount(frameCount), format(format) {}

    void encode(const string& inputPath, const string& outputPath) {
        // Ensure input path has .yuv extension
        string inputFile = inputPath;
        if (inputFile.substr(inputFile.length() - 4) != ".yuv") {
            inputFile += ".yuv";
        }

        ifstream input(inputFile, ios::binary);
        if (!input.is_open()) {
            throw runtime_error("Could not open input file: " + inputFile);
        }

        // Save metadata
        ofstream metaFile(outputPath + "_meta.txt");
        metaFile << width << " " << height << " " << frameCount << " " << format << endl;
        metaFile.close();

        cout << "Processing " << frameCount << " frames..." << endl;
        
        for (int frame = 0; frame < frameCount; ++frame) {
            Mat yuvFrame = readYUVFrame(input);
            string frameOutputPath = outputPath + "_frame" + to_string(frame);
            imageCodec.encode(yuvFrame, frameOutputPath);
            
            if (frame % 10 == 0) { // Progress indicator
                cout << "Encoded frame " << frame << "/" << frameCount << endl;
            }
        }
        input.close();
    }

    void decode(const string& inputPath, const string& outputPath) {
        // Ensure output path has .yuv extension
        string outputFile = outputPath;
        if (outputFile.substr(outputFile.length() - 4) != ".yuv") {
            outputFile += ".yuv";
        }

        ifstream metaFile(inputPath + "_meta.txt");
        if (!metaFile.is_open()) {
            throw runtime_error("Could not open metadata file: " + inputPath + "_meta.txt");
        }
        metaFile >> width >> height >> frameCount >> format;
        metaFile.close();

        ofstream output(outputFile, ios::binary);
        if (!output.is_open()) {
            throw runtime_error("Could not open output file: " + outputFile);
        }

        cout << "Decoding " << frameCount << " frames..." << endl;

        for (int frame = 0; frame < frameCount; ++frame) {
            string frameInputPath = inputPath + "_frame" + to_string(frame);
            Mat decodedFrame = imageCodec.decode(frameInputPath);
            writeYUVFrame(decodedFrame, output);

            if (frame % 10 == 0) { // Progress indicator
                cout << "Decoded frame " << frame << "/" << frameCount << endl;
            }
        }
        output.close();
        cout << "Finished decoding. Output saved to: " << outputFile << endl;
    }
};