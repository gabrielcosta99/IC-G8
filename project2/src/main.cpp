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

void handleIntraFrameVideo(const string &videoPath, int m) {
    IntraFrameVideoCodec codec(m, "encoded_video");
    codec.encode(videoPath); // Encode video

    // Decode video
    codec.decode("decoded_video.avi");
    cout << "Intra-frame video encoding and decoding completed!" << endl;
}

void handleInterFrameVideo(const string &videoPath, int m, int iFrameInterval, int blockSize, int searchRange) {
    InterFrameVideoCodec codec(m, iFrameInterval, blockSize, searchRange, "encoded_inter_video");
    codec.encode(videoPath); // Encode video

    // Decode video
    codec.decode("decoded_inter_video.avi");
    cout << "Inter-frame video encoding and decoding completed!" << endl;
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

void handleIntraFrameVideoCompression(const string &videoPath, const string &outputPath, int m) {
    IntraFrameVideoCodec codec(m, "encoded_video");

    // Compress and save the video
    codec.saveCompressedVideo(videoPath, outputPath);

    cout << "Compressed video saved to: " << outputPath << endl;
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
        cerr << "  image <image_path>: Encode and decode an image." << endl;
        cerr << "  image_compress <image_path> <output_path>: Compress an image." << endl;
        cerr << "  intra <video_path>: Perform intra-frame video coding." << endl;
        cerr << "  intra_compress <video_path> <output_path>: Compress a video (intra-frame)." << endl;
        cerr << "  inter <video_path> <iFrameInterval> <blockSize> <searchRange>: Perform inter-frame video coding." << endl;
        cerr << "  inter_compress <video_path> <output_path> <iFrameInterval> <blockSize> <searchRange>: Compress a video (inter-frame)." << endl;
        return -1;
    }

    string mode = argv[1];
    string inputPath = argv[2];
    int m = 4; // Golomb parameter

    if (mode == "image") {
        handleImage(inputPath, m);
    } else if (mode == "image_compress") {
        if (argc < 4) {
            cerr << "Usage: " << argv[0] << " image_compress <image_path> <output_path>" << endl;
            return -1;
        }
        string outputPath = argv[3];
        handleImageCompression(inputPath, outputPath, m);
    } else if (mode == "intra") {
        handleIntraFrameVideo(inputPath, m);
    } else if (mode == "intra_compress") {
        if (argc < 4) {
            cerr << "Usage: " << argv[0] << " intra_compress <video_path> <output_path>" << endl;
            return -1;
        }
        string outputPath = argv[3];
        handleIntraFrameVideoCompression(inputPath, outputPath, m);
    } else if (mode == "inter") {
        if (argc < 6) {
            cerr << "Usage: " << argv[0] << " inter <video_path> <iFrameInterval> <blockSize> <searchRange>" << endl;
            return -1;
        }
        int iFrameInterval = stoi(argv[3]);
        int blockSize = stoi(argv[4]);
        int searchRange = stoi(argv[5]);
        handleInterFrameVideo(inputPath, m, iFrameInterval, blockSize, searchRange);
    } else if (mode == "inter_compress") {
        if (argc < 7) {
            cerr << "Usage: " << argv[0] << " inter_compress <video_path> <output_path> <iFrameInterval> <blockSize> <searchRange>" << endl;
            return -1;
        }
        string outputPath = argv[3];
        int iFrameInterval = stoi(argv[4]);
        int blockSize = stoi(argv[5]);
        int searchRange = stoi(argv[6]);
        handleIntraFrameVideoCompression(inputPath, outputPath, m);
    } else {
        cerr << "Invalid mode. Use 'image', 'image_compress', 'intra', 'intra_compress', 'inter', or 'inter_compress'." << endl;
        return -1;
    }

    return 0;
}

