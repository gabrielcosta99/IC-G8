//#ifdef INTRA_FRAME_VIDEO_CODEC_H
//#define INTRA_FRAME_VIDEO_CODEC_H

#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include "Image_codec.h"





using namespace cv;
using namespace std;

class IntraFrameVideoCodec {
private:
    int m; // Golomb parameter
    string videoFilename;

    void writeFrameData(const Mat &frame, Golomb &encoder) {
        vector<Mat> channels;
        split(frame, channels);
        
        // For each channel
        for (const Mat &channel : channels) {
            // Calculate and encode residuals
            Mat residuals = ImageCodec(m).calculateResiduals(channel);
            for (int y = 0; y < residuals.rows; ++y) {
                for (int x = 0; x < residuals.cols; ++x) {
                    encoder.encode(residuals.at<int>(y, x));
                }
            }
        }
    }

    Mat readFrameData(Golomb &decoder, int rows, int cols, int channels) {
        vector<Mat> decodedChannels(channels);
        
        // For each channel
        for (int c = 0; c < channels; ++c) {
            Mat residuals = Mat::zeros(Size(cols, rows), CV_32S);
            // Read residuals
            for (int y = 0; y < rows; ++y) {
                for (int x = 0; x < cols; ++x) {
                    residuals.at<int>(y, x) = decoder.decode_val();
                }
            }
            decodedChannels[c] = ImageCodec(m).reconstructChannel(residuals);
        }
        
        Mat frame;
        merge(decodedChannels, frame);
        return frame;
    }

public:
    IntraFrameVideoCodec(int golombParam, const string &outputFile)
        : m(golombParam), videoFilename(outputFile) {}

    void encode(const string &inputVideo) {
        VideoCapture capture(inputVideo);
        if (!capture.isOpened()) {
            cerr << "Error: Unable to open video file: " << inputVideo << endl;
            return;
        }

        // Write metadata
        ofstream metaFile(videoFilename + "_meta.txt");
        metaFile << capture.get(CAP_PROP_FRAME_WIDTH) << " "
                 << capture.get(CAP_PROP_FRAME_HEIGHT) << " "
                 << capture.get(CAP_PROP_FPS) << " "
                 << capture.get(CAP_PROP_FRAME_COUNT) << " "
                 << capture.get(CAP_PROP_CHANNEL) << endl;
        metaFile.close();

        // Create single Golomb encoder for all frames
        Golomb encoder(m, false, videoFilename + ".bin");

        while (true) {
            Mat frame;
            capture >> frame;
            if (frame.empty()) break;
            writeFrameData(frame, encoder);
        }

        encoder.end();
        capture.release();
    }

    void decode(const string &outputVideo) {
        // Read metadata
        ifstream metaFile(videoFilename + "_meta.txt");
        int width, height, fps, frameCount, channels;
        metaFile >> width >> height >> fps >> frameCount >> channels;
        metaFile.close();

        VideoWriter writer(outputVideo, VideoWriter::fourcc('M','J','P','G'), 
                         fps, Size(width, height), channels > 1);

        // Create single Golomb decoder for all frames
        Golomb decoder(m, true, videoFilename + ".bin");

        for (int i = 0; i < frameCount; i++) {
            Mat frame = readFrameData(decoder, height, width, channels);
            writer.write(frame);
        }

        writer.release();
    }

    void saveCompressedVideo(const string &inputVideo, const string &outputVideo) {
    VideoCapture capture(inputVideo);
    if (!capture.isOpened()) {
        cerr << "Failed to open video file: " << inputVideo << endl;
        return;
    }

    int width = capture.get(CAP_PROP_FRAME_WIDTH);
    int height = capture.get(CAP_PROP_FRAME_HEIGHT);
    double fps = capture.get(CAP_PROP_FPS);
    int fourcc = VideoWriter::fourcc('M', 'J', 'P', 'G');

    VideoWriter writer(outputVideo, fourcc, fps, Size(width, height), true);
    if (!writer.isOpened()) {
        cerr << "Failed to open output video file: " << outputVideo << endl;
        return;
    }

    while (true) {
        Mat frame;
        capture >> frame; // Read next frame
        if (frame.empty())
            break;

        ImageCodec codec(m);
        vector<Mat> channels;
        split(frame, channels);

        for (int i = 0; i < channels.size(); ++i) {
            Mat residuals = codec.calculateResiduals(channels[i]);
            channels[i] = codec.reconstructChannel(residuals);
        }

        Mat compressedFrame;
        merge(channels, compressedFrame);
        writer.write(compressedFrame); // Write reconstructed frame
    }

    capture.release();
    writer.release();

    cout << "Compressed video saved to: " << outputVideo << endl;
}

};

//#endif // INTRA_FRAME_VIDEO_CODEC_H