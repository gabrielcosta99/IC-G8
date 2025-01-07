#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Image_codec.h"
#include "Golomb.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace cv;
using namespace std;

class IntraFrameVideoCodec {
private:
    ImageCodec imageCodec;
    int width;
    int height;
    int frameCount;
    int m;  // Golomb parameter~
    string y4mHeader;
    int sourceFormat;

    // Calculate frame sizes for YUV420p
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

    // Modified to accept generic istream instead of specific ifstream
    bool parseY4MHeader(istream& file) {
        string header;
        getline(file, header);
        
        if (header.substr(0, 10) != "YUV4MPEG2 ") {
            return false;
        }

        // Parse format before modifying header
        sourceFormat = parseFormat(header);
        
        // Modify header to always indicate 420 format
        size_t c_pos = header.find("C");
        if (c_pos != string::npos) {
            size_t space_pos = header.find(' ', c_pos);
            header = header.substr(0, c_pos) + "C420" + 
                    (space_pos != string::npos ? header.substr(space_pos) : "");
        } else {
            header += " C420";
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

public:
    IntraFrameVideoCodec(int m, int width = 0, int height = 0, int frameCount = 0) 
        : imageCodec(m), width(width), height(height), m(m) {
        if (width != 0 && height != 0) {
            validateDimensions();
        }
    }

    void encode(const string& inputPath, const string& outputPath) {
        ifstream input(inputPath, ios::binary);
        if (!input) {
            throw runtime_error("Could not open input file: " + inputPath);
        }

        // Parse Y4M header (now also sets sourceFormat)
        if (!parseY4MHeader(input)) {
            throw runtime_error("Invalid Y4M header");
        }

        // Count frames if not provided
        if (frameCount == 0) {
            size_t frameSize = getYSize() + 2 * getSourceUVSize() + 6; // +6 for "FRAME\n"
            streampos current = input.tellg();
            input.seekg(0, ios::end);
            size_t fileSize = static_cast<size_t>(input.tellg()) - static_cast<size_t>(current);
            frameCount = fileSize / frameSize;
            input.seekg(current, ios::beg);
        }

        // Write metadata
        ofstream meta(outputPath + ".meta");
        if (!meta) {
            throw runtime_error("Could not create metadata file");
        }
        meta << y4mHeader;  // Store original Y4M header
        meta << frameCount << endl;
        meta.close();

        // Create Golomb encoder
        Golomb golomb(m, false, outputPath + ".bin", 1);

        cout << "Encoding " << frameCount << " frames..." << endl;
        
        for (int f = 0; f < frameCount; ++f) {
            vector<Mat> planes = readY4MFrame(input);
            
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
        
        // Read Y4M header from metadata
        getline(meta, y4mHeader);
        meta >> frameCount;
        
        // Parse dimensions from Y4M header
        stringstream headerStream(y4mHeader);
        if (!parseY4MHeader(headerStream)) {
            throw runtime_error("Invalid Y4M header in metadata");
        }
        
        meta.close();
        validateDimensions();

        // Create Golomb decoder
        Golomb golomb(m, true, inputPath + ".bin", 1);

        // Open output file and write Y4M header
        ofstream output(outputPath, ios::binary);
        if (!output) {
            throw runtime_error("Could not open output file");
        }
        output << y4mHeader;

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
            
            // Write reconstructed Y4M frame
            writeY4MFrame(reconstructedPlanes, output);
            
            if (f % 10 == 0) {
                cout << "Decoded frame " << f << "/" << frameCount << endl;
            }
        }
        
        cout << "Decoding complete" << endl;
    }
};