#include "../include/Image_codec.h"
#include "../include/intra_frame_video_codec.h"
#include "../include/inter_frame_video_codec.h"
#include "../include/lossy_inter_video_codec.h"
#include "../include/compare.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <filesystem>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

void handleImage(const string &imagePath, int m) {
    Mat image = imread(imagePath, IMREAD_COLOR); // Load the image
    if (image.empty()) {
        cerr << "Failed to load image: " << imagePath << endl;
        return;
    }

    ImageCodec codec(m);

    // Encode and decode the image
    codec.encode(image, "encoded_image");
    Mat decodedImage = codec.decode("encoded_image");

    // Display original and decoded images
    imshow("Original Image", image);
    imshow("Decoded Image", decodedImage);

    // Calculate and display MSE/PSNR
    Mat diff;
    absdiff(image, decodedImage, diff);
    double mse = mean(diff.mul(diff))[0];
    double psnr = 10 * log10((255 * 255) / mse);

    cout << "MSE: " << mse << endl;
    cout << "PSNR: " << psnr << " dB" << endl;

    waitKey(0);
}

void handleImageCompression(const string &imagePath, const string &outputPath, int m) {
    Mat image = imread(imagePath, IMREAD_COLOR); // Load the image
    if (image.empty()) {
        cerr << "Failed to load image: " << imagePath << endl;
        return;
    }

    ImageCodec codec(m);

    // Save the compressed version of the image
    codec.saveCompressedImage(image, outputPath);

    cout << "Compressed image saved to: " << outputPath << endl;

    // Compare original and compressed files
    Compare compare;
    compare.compareFiles(imagePath, outputPath);
}

void handleIntraFrameVideoCompression(const string &videoPath, const string &outputPath, 
                                    int width, int height, int frameCount, int m) {
    try {
        cout << "Starting video compression..." << endl;
        cout << "Parameters: " << width << "x" << height << ", " << frameCount << " frames" << endl;
        
        IntraFrameVideoCodec codec(m, width, height, frameCount);
        
        // Encode the video
        cout << "Encoding video..." << endl;
        codec.encode(videoPath, outputPath);
        cout << "Encoding complete." << endl;
        
        // Decode the video to verify
        cout << "Decoding video for verification..." << endl;
        string decodedPath = outputPath + "_decoded.yuv";
        codec.decode(outputPath, decodedPath);
        cout << "Decoding complete. Output saved to: " << decodedPath << endl;
        
    } catch (const exception& e) {
        cerr << "Error during video compression: " << e.what() << endl;
    }
}

void handleInterFrameVideoCompression (const string &videoPath, const string &outputPath, int m, int width, int height, int frameCount, int iFrameInterval, int blockSize, int searchRange, string format) {
    try {
        cout << "Starting video compression..." << endl;
        cout << "Parameters: " << iFrameInterval << " I-frame interval, " << blockSize << " block size, " << searchRange << " search range" << endl;
        
        InterFrameVideoCodec codec(m, width, height, frameCount, iFrameInterval, blockSize, searchRange);
        
        // Encode the video
        cout << "Encoding video..." << endl;
        codec.encode(videoPath, outputPath);
        cout << "Encoding complete." << endl;
        
        // Decode the video to verify
        cout << "Decoding video for verification..." << endl;
        string decodedPath = outputPath + "_decoded.yuv";
        codec.decode(outputPath, decodedPath);
        cout << "Decoding complete. Output saved to: " << decodedPath << endl;
        
    } catch (const exception& e) {
        cerr << "Error during video compression: " << e.what() << endl;
    }
}

void handleInterLossyFrameVideoCompression(const string &videoPath, const string &outputPath, int m, int width, int height, int frameCount, int iFrameInterval, int blockSize, int searchRange, int quantizationLevel) {
    try {
        cout << "Starting video compression..." << endl;
        cout << "Parameters: " << iFrameInterval << " I-frame interval, " << blockSize << " block size, " << searchRange << " search range" << endl;
        
        InterFrameVideoLossyCodec codec(m, width, height, frameCount, iFrameInterval, blockSize, searchRange, quantizationLevel);
        
        // Encode the video
        cout << "Encoding video..." << endl;
        codec.encode(videoPath, outputPath);
        cout << "Encoding complete." << endl;
        
        // Decode the video to verify
        cout << "Decoding video for verification..." << endl;
        string decodedPath = outputPath + "_decoded.yuv";
        codec.decode(outputPath, decodedPath);
        cout << "Decoding complete. Output saved to: " << decodedPath << endl;
        
    } catch (const exception& e) {
        cerr << "Error during video compression: " << e.what() << endl;
    }
}


//void handleIntraFrameVideoCompression(const string &videoPath, const string &outputPath, int m) {
//    IntraFrameVideoCodec codec(m, "encoded_video");
//
//    // Compress and save the video
//    codec.saveCompressedVideo(videoPath, outputPath);
//
//    cout << "Compressed video saved to: " << outputPath << endl;
//}

// void handleInterFrameVideoCompression(const string &videoPath, const string &outputPath, int m, int iFrameInterval, int blockSize, int searchRange) {
//     InterFrameVideoCodec codec(m, iFrameInterval, blockSize, searchRange, "encoded_inter_video");

//     // Compress and save the video
//     codec.saveCompressedVideo(videoPath, outputPath);

//     cout << "Compressed video saved to: " << outputPath << endl;
// }


// void saveCompressedVideo(const string &videoPath, const string &outputPath, int m, int iFrameInterval, int blockSize, int searchRange) {
//     InterFrameVideoCodec codec(m, iFrameInterval, blockSize, searchRange, "encoded_inter_video");

//     // Compress and save the video
//     codec.saveCompressedVideo(videoPath, outputPath);

//     cout << "Compressed video saved to: " << outputPath << endl;
// }

void displayMenu() {
    cout << "Select mode:" << endl;
    cout << "1. Image Encoding/Decoding" << endl;
    cout << "2. Image Compression" << endl;
    cout << "3. Intra-frame Video Compression" << endl;
    cout << "4. Inter-frame Video Compression" << endl;
    cout << "5. Inter-frame Lossy Video Compression" << endl;
    cout << "6. Exit" << endl;
    cout << "Enter your choice: ";
}

void listFiles(const string& directory) {
    for (const auto& entry : fs::directory_iterator(directory)) {
        cout << entry.path().filename().string() << endl;
    }
}

string chooseFile(const string& directory) {
    listFiles(directory);
    string filename;
    cout << "Enter the filename from the above list: ";
    cin >> filename;
    return directory + "/" + filename;
}

void handleChoice(int choice) {
    string inputPath;
    int m = 4; // Default Golomb parameter

    switch (choice) {
        case 1:
            inputPath = chooseFile("../images");
            handleImage(inputPath, m);
            break;
        case 2: {
            inputPath = chooseFile("../images");
            fs::path inputFilePath(inputPath);
            string outputPath = (inputFilePath.parent_path() / ("compressed_" + inputFilePath.filename().string())).string();
            handleImageCompression(inputPath, outputPath, m);
            break;
        }
        case 3: {
            inputPath = chooseFile("../videos");
            fs::path inputFilePath(inputPath);
            string outputPath = (inputFilePath.parent_path() / ("encoded_" + inputFilePath.stem().string())).string();
            int width, height, frameCount;
            cout << "Enter width: ";
            cin >> width;
            cout << "Enter height: ";
            cin >> height;
            cout << "Enter frame count: ";
            cin >> frameCount;
            handleIntraFrameVideoCompression(inputPath, outputPath, width, height, frameCount, m);
            Compare compare;
            compare.compareFiles(inputPath, outputPath + "_decoded.yuv");
            break;
        }
        case 4: {
            inputPath = chooseFile("../videos");
            fs::path inputFilePath(inputPath);
            string outputPath = (inputFilePath.parent_path() / ("encoded_inter_" + inputFilePath.stem().string())).string();
            int width, height, frameCount, iFrameInterval, blockSize, searchRange;
            cout << "Enter width: ";
            cin >> width;
            cout << "Enter height: ";
            cin >> height;
            cout << "Enter frame count: ";
            cin >> frameCount;
            cout << "Enter I-frame interval: ";
            cin >> iFrameInterval;
            cout << "Enter block size: ";
            cin >> blockSize;
            cout << "Enter search range: ";
            cin >> searchRange;
            handleInterFrameVideoCompression(inputPath, outputPath, m, width, height, frameCount, iFrameInterval, blockSize, searchRange, "420");
            Compare compare;
            compare.compareFiles(inputPath, outputPath + "_decoded.yuv");
            break;
        }
        case 5: {
            inputPath = chooseFile("../videos");
            fs::path inputFilePath(inputPath);
            string outputPath = (inputFilePath.parent_path() / ("encoded_lossy_" + inputFilePath.stem().string())).string();
            int width, height, frameCount, iFrameInterval, blockSize, searchRange, quantizationLevel;
            cout << "Enter width: ";
            cin >> width;
            cout << "Enter height: ";
            cin >> height;
            cout << "Enter frame count: ";
            cin >> frameCount;
            cout << "Enter I-frame interval: ";
            cin >> iFrameInterval;
            cout << "Enter block size: ";
            cin >> blockSize;
            cout << "Enter search range: ";
            cin >> searchRange;
            cout << "Enter quantization level: ";
            cin >> quantizationLevel;
            handleInterLossyFrameVideoCompression(inputPath, outputPath, m, width, height, frameCount, iFrameInterval, blockSize, searchRange, quantizationLevel);
            Compare compare;
            compare.compareFiles(inputPath, outputPath + "_decoded.yuv");
            break;
        }
        case 6:
            cout << "Exiting program." << endl;
            exit(0);
        default:
            cerr << "Invalid choice. Exiting." << endl;
            exit(-1);
    }
}

int main() {

    while (true) {
        int choice;
        displayMenu();
        cin >> choice;

        handleChoice(choice);
    
    }


    return 0;
}
