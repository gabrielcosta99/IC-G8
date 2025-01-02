#include "../include/Image_codec.h"
#include "../include/intra_frame_video_codec.h"
#include "../include/inter_frame_video_codec.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>




using namespace cv;
using namespace std;

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
        
        InterFrameVideoCodec codec(m, width, height, frameCount, iFrameInterval, blockSize, searchRange, format);
        
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


int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <mode> <input_path> [additional parameters]" << endl;
        cerr << "Modes:" << endl;
        cerr << "  image <image_path>: Encode and decode an image" << endl;
        cerr << "  image_compress <image_path> <output_path>: Compress an image" << endl;
        cerr << "  intra_compress <video_path> <output_path> <width> <height> <frame_count>: Compress a YUV video" << endl;
        cerr << "  inter_compress <video_path> <output_path> <width> <height> <frame_count> <iFrameInterval> <blockSize> <searchRange> <format>: Compress a YUV video" << endl;
        return -1;
    }

    string mode = argv[1];
    string inputPath = argv[2];
    int m = 4; // Default Golomb parameter

    if (mode == "image") {
        handleImage(inputPath, m);
    } 
    else if (mode == "image_compress") {
        if (argc < 4) {
            cerr << "Usage: " << argv[0] << " image_compress <image_path> <output_path>" << endl;
            return -1;
        }
        string outputPath = argv[3];
        handleImageCompression(inputPath, outputPath, m);
    } 
    else if (mode == "intra_compress") {
        if (argc < 7) {
            cerr << "Usage: " << argv[0] << " intra_compress <video_path> <output_path> <width> <height> <frame_count>" << endl;
            return -1;
        }
        string outputPath = argv[3];
        int width = stoi(argv[4]);
        int height = stoi(argv[5]);
        int frameCount = stoi(argv[6]);
        
        // Validate parameters
        if (width <= 0 || height <= 0 || frameCount <= 0) {
            cerr << "Invalid parameters. Width, height, and frame count must be positive." << endl;
            return -1;
        }
        
        handleIntraFrameVideoCompression(inputPath, outputPath, width, height, frameCount, m);
    }
    else if (mode == "inter_compress") {
        if (argc < 11) {
            cerr << "Usage: " << argv[0] << " inter_compress <video_path> <output_path> <width> <height> <frame_count> <iFrameInterval> <blockSize> <searchRange> <format>" << endl;
            return -1;
        }
        string outputPath = argv[3];
        int width = stoi(argv[4]);
        int height = stoi(argv[5]);
        int frameCount = stoi(argv[6]);
        int iFrameInterval = stoi(argv[7]);
        int blockSize = stoi(argv[8]);
        int searchRange = stoi(argv[9]);
        string format = argv[10];
        
        // Validate parameters
        if (width <= 0 || height <= 0 || frameCount <= 0 || iFrameInterval <= 0 || blockSize <= 0 || searchRange <= 0) {
            cerr << "Invalid parameters. Width, height, frame count, I-frame interval, block size, and search range must be positive." << endl;
            return -1;
        }
        
        handleInterFrameVideoCompression(inputPath, outputPath, m, width, height, frameCount, iFrameInterval, blockSize, searchRange, format);
    }
    else {
        cerr << "Invalid mode. Use 'image', 'image_compress', or 'intra_compress'." << endl;
        return -1;
    }

    return 0;
}
