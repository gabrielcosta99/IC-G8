#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include "Image_codec.h"


using namespace cv;
using namespace std;

class InterFrameVideoCodec {
private:
    int m;           // Golomb parameter
    int iFrameInterval;
    int blockSize;
    int searchRange;
    string videoFilename;
    Mat previousFrame;  // Add this as class member

    Mat calculateResidual(const Mat &current, const Mat &reference, vector<Point2i> &motionVectors) {
        Mat residual = Mat::zeros(current.size(), CV_32S);

        for (int y = 0; y < current.rows; y += blockSize) {
            for (int x = 0; x < current.cols; x += blockSize) {
                Rect block(x, y, blockSize, blockSize);
                Mat currentBlock = current(block);

                // Motion estimation
                int minError = INT_MAX;
                Point2i bestMatch(0, 0);

                for (int dy = -searchRange; dy <= searchRange; ++dy) {
                    for (int dx = -searchRange; dx <= searchRange; ++dx) {
                        Rect searchBlock(x + dx, y + dy, blockSize, blockSize);
                        if (searchBlock.x < 0 || searchBlock.y < 0 || searchBlock.br().x > reference.cols || searchBlock.br().y > reference.rows)
                            continue;

                        Mat referenceBlock = reference(searchBlock);
                        int error = norm(currentBlock - referenceBlock, NORM_L1);

                        if (error < minError) {
                            minError = error;
                            bestMatch = Point2i(dx, dy);
                        }
                    }
                }

                motionVectors.push_back(bestMatch);

                // Compute residual
                Rect matchedBlock(x + bestMatch.x, y + bestMatch.y, blockSize, blockSize);
                Mat refBlock = reference(matchedBlock);
                currentBlock.convertTo(currentBlock, CV_32S);
                refBlock.convertTo(refBlock, CV_32S);
                Mat residualBlock = currentBlock - refBlock;
                residualBlock.copyTo(residual(block));
            }
        }

        return residual;
    }

    void writeFrameData(const Mat &frame, const vector<Point2i> &motionVectors, 
                       const Mat &residual, bool isIFrame, Golomb &encoder) {
        // Write frame type (I-frame = 1, P-frame = 0)
        encoder.encode(isIFrame ? 1 : 0);

        if (isIFrame) {
            // For I-frame, encode the frame directly using ImageCodec
            vector<Mat> channels;
            split(frame, channels);
            vector<Mat> residuals(channels.size());
            
            for (size_t i = 0; i < channels.size(); ++i) {
                residuals[i] = ImageCodec(m).calculateResiduals(channels[i]);
                // Encode each pixel of the residual
                for (int y = 0; y < residuals[i].rows; ++y) {
                    for (int x = 0; x < residuals[i].cols; ++x) {
                        encoder.encode(residuals[i].at<int>(y, x));
                    }
                }
            }
        } else {
            // For P-frame, encode motion vectors and residual
            // Encode number of motion vectors
            encoder.encode(motionVectors.size());
            
            // Encode motion vectors
            for (const auto &mv : motionVectors) {
                encoder.encode(mv.x);
                encoder.encode(mv.y);
            }

            // Encode residual data
            for (int y = 0; y < residual.rows; ++y) {
                for (int x = 0; x < residual.cols; ++x) {
                    encoder.encode(residual.at<int>(y, x));
                }
            }
        }
    }

    Mat readFrameData(Golomb &decoder, const Size &frameSize, int channels) {
        // Read frame type
        bool isIFrame = decoder.decode_val() == 1;
        
        if (isIFrame) {
            vector<Mat> decodedChannels(channels);
            for (int c = 0; c < channels; ++c) {
                Mat residuals = Mat::zeros(frameSize, CV_32S);
                for (int y = 0; y < frameSize.height; ++y) {
                    for (int x = 0; x < frameSize.width; ++x) {
                        residuals.at<int>(y, x) = decoder.decode_val();
                    }
                }
                decodedChannels[c] = ImageCodec(m).reconstructChannel(residuals);
            }
            
            Mat frame;
            merge(decodedChannels, frame);
            previousFrame = frame.clone();  // Update previousFrame
            return frame;
        } else {
            // Read number of motion vectors
            int numVectors = decoder.decode_val();
            vector<Point2i> motionVectors;
            
            // Read motion vectors
            for (int i = 0; i < numVectors; ++i) {
                int x = decoder.decode_val();
                int y = decoder.decode_val();
                motionVectors.push_back(Point2i(x, y));
            }
            
            // Read residual data
            Mat residual = Mat::zeros(frameSize, CV_32S);
            for (int y = 0; y < frameSize.height; ++y) {
                for (int x = 0; x < frameSize.width; ++x) {
                    residual.at<int>(y, x) = decoder.decode_val();
                }
            }
            
            // Reconstruct frame using motion compensation
            Mat reconstructed = applyMotionCompensation(residual, previousFrame, blockSize, searchRange);
            previousFrame = reconstructed.clone();  // Update previousFrame
            return reconstructed;
        }
    }

public:
    InterFrameVideoCodec(int golombParam, int iFrameInterval, int blockSize, int searchRange, const string &outputFile)
        : m(golombParam), iFrameInterval(iFrameInterval), blockSize(blockSize), searchRange(searchRange), videoFilename(outputFile) {}

    Mat applyMotionCompensation(const Mat &current, const Mat &reference, int blockSize, int searchRange) {
        Mat motionCompensatedFrame = Mat::zeros(current.size(), CV_8U);

        for (int y = 0; y < current.rows; y += blockSize) {
            for (int x = 0; x < current.cols; x += blockSize) {
                Rect block(x, y, blockSize, blockSize);
                Mat currentBlock = current(block);

                Point2i motionVector(0, 0); // Default motion vector
                if (y > 0 && x > 0) {
                    ifstream mvFile(videoFilename + "_frame_" + to_string(y) + "_mv.txt");
                    mvFile >> motionVector.x >> motionVector.y;
                    mvFile.close();
                }

                Rect refBlock(x + motionVector.x, y + motionVector.y, blockSize, blockSize);
                Mat refeBlock = reference(refBlock);
                refeBlock.copyTo(motionCompensatedFrame(block));
            }
        }

        return motionCompensatedFrame;
    }

    void encode(const string &inputVideo) {
        VideoCapture capture(inputVideo);
        if (!capture.isOpened()) {
            cerr << "Error: Unable to open video file: " << inputVideo << endl;
            return;
        }

        // Write video metadata
        ofstream metaFile(videoFilename + "_meta.txt");
        metaFile << capture.get(CAP_PROP_FRAME_WIDTH) << " "
                 << capture.get(CAP_PROP_FRAME_HEIGHT) << " "
                 << capture.get(CAP_PROP_FPS) << " "
                 << capture.get(CAP_PROP_FRAME_COUNT) << " "
                 << capture.get(CAP_PROP_CHANNEL) << endl;
        metaFile.close();

        Golomb encoder(m, false, videoFilename + ".bin");
        Mat previousFrame;
        int frameIndex = 0;

        while (true) {
            Mat frame;
            capture >> frame;
            if (frame.empty()) break;

            if (frameIndex % iFrameInterval == 0) {
                // I-frame
                writeFrameData(frame, vector<Point2i>(), Mat(), true, encoder);
            } else {
                // P-frame
                vector<Point2i> motionVectors;
                Mat residual = calculateResidual(frame, previousFrame, motionVectors);
                writeFrameData(frame, motionVectors, residual, false, encoder);
            }

            previousFrame = frame.clone();
            frameIndex++;
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

        Golomb decoder(m, true, videoFilename + ".bin");
        Mat previousFrame;

        for (int i = 0; i < frameCount; i++) {
            Mat frame = readFrameData(decoder, Size(width, height), channels);
            writer.write(frame);
            previousFrame = frame.clone();
        }

        writer.release();
    }

    void saveCompressedInterFrameVideo(const string &inputVideo, const string &outputVideo, int iFrameInterval, int blockSize, int searchRange) {
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

        Mat previousFrame;
        int frameIndex = 0;

        while (true) {
            Mat frame;
            capture >> frame; // Read next frame
            if (frame.empty())
                break;

            if (frameIndex % iFrameInterval == 0) {
                // Intra-frame: encode independently
                ImageCodec codec(m);
                vector<Mat> channels;
                split(frame, channels);

                for (int i = 0; i < channels.size(); ++i) {
                    Mat residuals = codec.calculateResiduals(channels[i]);
                    channels[i] = codec.reconstructChannel(residuals);
                }

                Mat compressedFrame;
                merge(channels, compressedFrame);
                writer.write(compressedFrame);

            } else {
                // Inter-frame: motion compensation
                Mat motionCompensatedFrame = applyMotionCompensation(frame, previousFrame, blockSize, searchRange);
                writer.write(motionCompensatedFrame);
            }

            previousFrame = frame.clone();
            frameIndex++;
        }

        capture.release();
        writer.release();

        cout << "Compressed inter-frame video saved to: " << outputVideo << endl;
    }


};

//#endif //  INTER_FRAME_VIDEO_CODEC_H