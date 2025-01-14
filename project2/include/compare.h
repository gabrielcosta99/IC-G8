//
// Created by weza on 1/6/25.
//

#ifndef COMPARE_H
#define COMPARE_H

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>


/**
 * @brief Compare class
 * class to compare two files and show the compression ratio
 * bit by bit
 */
class Compare {
public:
    /**
     * @brief Compare two files
     * @param originalPath path to the original file
     * @param compressedPath path to the compressed file
     */
    void compareFiles(const std::string &originalPath, const std::string &compressedPath) {
        std::ifstream originalFile(originalPath, std::ios::binary);
        std::ifstream compressedFile(compressedPath, std::ios::binary);

        if (!originalFile || !compressedFile) {
            throw std::runtime_error("Could not open files");
        }

        // Get file sizes
        originalFile.seekg(0, std::ios::end);
        compressedFile.seekg(0, std::ios::end);
        size_t originalSize = originalFile.tellg();
        size_t compressedSize = compressedFile.tellg();
        originalFile.seekg(0, std::ios::beg);
        compressedFile.seekg(0, std::ios::beg);

        // Read files bit by bit
        size_t totalBits = std::max(originalSize, compressedSize) * 8;
        size_t differentBits = 0;
        char originalByte, compressedByte;
        while (originalFile.get(originalByte) && compressedFile.get(compressedByte)) {
            for (int i = 0; i < 8; ++i) {
                if ((originalByte & (1 << i)) != (compressedByte & (1 << i))) {
                    ++differentBits;
                }
            }
        }

        // Calculate compression ratio and error rate
        double compressionRatio = static_cast<double>(compressedSize) / originalSize;
        double errorRate = (static_cast<double>(differentBits) / totalBits) * 100.0; // Convert to percentage

        printf("\n");
        std::cout << "Original size: " << originalSize << " bytes" << std::endl;
        std::cout << "Compressed size: " << compressedSize << " bytes" << std::endl;
        std::cout << "Compression ratio: " << compressionRatio << std::endl;
        std::cout << "Bit error rate: " << errorRate << "%" << std::endl; // Display as percentage
        printf("\n");
    }

    /**
     * @brief Compare original and decoded files
     * @param original path to the original file
     * @param decoded path to the decoded file
     */
    void compareVideoFiles(const std::string& original, const std::string& decoded) {
        std::ifstream orig(original, std::ios::binary);
        std::ifstream dec(decoded, std::ios::binary);

        if (!orig || !dec) {
            std::cout << "Error opening files for comparison" << std::endl;
            return;
        }

        // Skip Y4M headers
        std::string header;
        std::getline(orig, header);
        std::getline(dec, header);

        char buf1, buf2;
        size_t totalBytes = 0;
        size_t diffBytes = 0;

        while (orig.get(buf1) && dec.get(buf2)) {
            if (buf1 != buf2) diffBytes++;
            totalBytes++;
        }

        std::cout << "Total bytes compared: " << totalBytes << std::endl;
        std::cout << "Different bytes: " << diffBytes << std::endl;
        std::cout << "Match percentage: " << 100.0 * (totalBytes - diffBytes) / totalBytes << "%" << std::endl;
    }

    void compareIntraFrameCompression(const std::string& originalPath, const std::string& compressedBasePath) {
        // Get original file size
        std::ifstream originalFile(originalPath, std::ios::binary | std::ios::ate);
        if (!originalFile) {
            throw std::runtime_error("Could not open original file: " + originalPath);
        }
        size_t originalSize = originalFile.tellg();

        // Get binary file size
        std::ifstream binFile(compressedBasePath + ".bin", std::ios::binary | std::ios::ate);
        if (!binFile) {
            throw std::runtime_error("Could not open binary file: " + compressedBasePath + ".bin");
        }
        size_t binSize = binFile.tellg();

        // Get metadata file size
        std::ifstream metaFile(compressedBasePath + ".meta", std::ios::binary | std::ios::ate);
        if (!metaFile) {
            throw std::runtime_error("Could not open metadata file: " + compressedBasePath + ".meta");
        }
        size_t metaSize = metaFile.tellg();

        // Calculate total compressed size
        size_t totalCompressedSize = binSize + metaSize;

        // Calculate compression metrics
        double compressionRatio = static_cast<double>(originalSize) / totalCompressedSize;
        double spaceSaving = (1.0 - static_cast<double>(totalCompressedSize) / originalSize) * 100.0;
        
        // Get frame count from metadata
        metaFile.seekg(0);
        std::string header;
        std::getline(metaFile, header); // Skip Y4M header
        int frameCount;
        metaFile >> frameCount;

        // Calculate per-frame metrics
        double bitsPerFrame = (totalCompressedSize * 8.0) / frameCount;
        
        // Print report
        std::cout << "\n=== Intra-Frame Compression Analysis ===\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Original file:     " << formatSize(originalSize) 
                 << " (" << originalSize << " bytes)\n";
        std::cout << "Compressed components:\n";
        std::cout << "  - Binary data:   " << formatSize(binSize) 
                 << " (" << binSize << " bytes)\n";
        std::cout << "  - Metadata:      " << formatSize(metaSize) 
                 << " (" << metaSize << " bytes)\n";
        std::cout << "  - Total:         " << formatSize(totalCompressedSize) 
                 << " (" << totalCompressedSize << " bytes)\n";
        std::cout << "\nCompression metrics:\n";
        std::cout << "  - Frame count:   " << frameCount << "\n";
        std::cout << "  - Ratio:         " << compressionRatio << ":1\n";
        std::cout << "  - Space saving:  " << spaceSaving << "%\n";
        std::cout << "  - Bits/frame:    " << static_cast<int>(bitsPerFrame) << "\n";
        std::cout << "====================================\n\n";
    }

    void compareInterFrameCompression(const std::string& originalPath, const std::string& compressedBasePath) {
        // Get original file size
        size_t originalSize = getFileSize(originalPath);
        
        // Get compressed components sizes
        size_t binSize = getFileSize(compressedBasePath + ".bin");
        size_t metaSize = getFileSize(compressedBasePath + ".meta");
        size_t totalCompressedSize = binSize + metaSize;

        // Read metadata for frame info
        std::ifstream metaFile(compressedBasePath + ".meta");
        std::string header;
        std::getline(metaFile, header);
        int frameCount, iFrameInterval, blockSize, searchRange;
        metaFile >> frameCount >> iFrameInterval >> blockSize >> searchRange;

        // Calculate metrics
        double compressionRatio = static_cast<double>(originalSize) / totalCompressedSize;
        double spaceSaving = (1.0 - static_cast<double>(totalCompressedSize) / originalSize) * 100.0;
        double bitsPerFrame = (totalCompressedSize * 8.0) / frameCount;
        double iFramesCount = std::ceil(static_cast<double>(frameCount) / iFrameInterval);
        double pFramesCount = frameCount - iFramesCount;

        // Print detailed report
        std::cout << "\n=== Inter-Frame Compression Analysis ===\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Original file:     " << formatSize(originalSize) 
                 << " (" << originalSize << " bytes)\n";
        std::cout << "Compressed components:\n";
        std::cout << "  - Binary data:   " << formatSize(binSize) 
                 << " (" << binSize << " bytes)\n";
        std::cout << "  - Metadata:      " << formatSize(metaSize) 
                 << " (" << metaSize << " bytes)\n";
        std::cout << "  - Total:         " << formatSize(totalCompressedSize) 
                 << " (" << totalCompressedSize << " bytes)\n";
        std::cout << "\nFrame statistics:\n";
        std::cout << "  - Total frames:  " << frameCount << "\n";
        std::cout << "  - I-frames:      " << static_cast<int>(iFramesCount) << "\n";
        std::cout << "  - P-frames:      " << static_cast<int>(pFramesCount) << "\n";
        std::cout << "  - Block size:    " << blockSize << "x" << blockSize << "\n";
        std::cout << "  - Search range:  " << searchRange << "\n";
        std::cout << "\nCompression metrics:\n";
        std::cout << "  - Ratio:         " << compressionRatio << ":1\n";
        std::cout << "  - Space saving:  " << spaceSaving << "%\n";
        std::cout << "  - Bits/frame:    " << static_cast<int>(bitsPerFrame) << "\n";
        std::cout << "=====================================\n\n";
    }

    void compareLossyInterFrameCompression(const std::string& originalPath, 
                                         const std::string& compressedBasePath,
                                         const std::string& decodedPath) {
        // Get file sizes
        size_t originalSize = getFileSize(originalPath);
        size_t binSize = getFileSize(compressedBasePath + ".bin");
        size_t metaSize = getFileSize(compressedBasePath + ".meta");
        size_t totalCompressedSize = binSize + metaSize;

        // Read metadata
        std::ifstream metaFile(compressedBasePath + ".meta");
        std::string header;
        std::getline(metaFile, header);
        int frameCount, iFrameInterval, blockSize, searchRange, qp;
        metaFile >> frameCount >> iFrameInterval >> blockSize >> searchRange >> qp;

        // Calculate compression metrics
        double compressionRatio = static_cast<double>(originalSize) / totalCompressedSize;
        double spaceSaving = (1.0 - static_cast<double>(totalCompressedSize) / originalSize) * 100.0;

        // Calculate quality metrics by comparing original and decoded files
        double psnr = calculatePSNR(originalPath, decodedPath);

        // Print report
        std::cout << "\n=== Lossy Inter-Frame Compression Analysis ===\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Original file:     " << formatSize(originalSize) 
                 << " (" << originalSize << " bytes)\n";
        std::cout << "Compressed components:\n";
        std::cout << "  - Binary data:   " << formatSize(binSize) 
                 << " (" << binSize << " bytes)\n";
        std::cout << "  - Metadata:      " << formatSize(metaSize) 
                 << " (" << metaSize << " bytes)\n";
        std::cout << "  - Total:         " << formatSize(totalCompressedSize) 
                 << " (" << totalCompressedSize << " bytes)\n";
        std::cout << "\nCompression parameters:\n";
        std::cout << "  - QP value:      " << qp << "\n";
        std::cout << "  - Block size:    " << blockSize << "x" << blockSize << "\n";
        std::cout << "  - Search range:  " << searchRange << "\n";
        std::cout << "  - I-frame int.:  " << iFrameInterval << "\n";
        std::cout << "\nQuality metrics:\n";
        std::cout << "  - PSNR:          " << psnr << " dB\n";
        std::cout << "\nCompression metrics:\n";
        std::cout << "  - Ratio:         " << compressionRatio << ":1\n";
        std::cout << "  - Space saving:  " << spaceSaving << "%\n";
        std::cout << "=========================================\n\n";
    }

private:
    std::string formatSize(size_t bytes) const {
        static const char* units[] = {"B", "KB", "MB", "GB"};
        double size = static_cast<double>(bytes);
        int unit = 0;
        
        while (size >= 1024.0 && unit < 3) {
            size /= 1024.0;
            unit++;
        }
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << units[unit];
        return ss.str();
    }

    size_t getFileSize(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) return 0;
        return file.tellg();
    }

    double calculatePSNR(const std::string& original, const std::string& decoded) {
        // Skip headers
        std::ifstream orig(original, std::ios::binary);
        std::ifstream dec(decoded, std::ios::binary);
        std::string header;
        std::getline(orig, header);
        std::getline(dec, header);

        double mse = 0;
        size_t totalPixels = 0;
        char byte1, byte2;

        while (orig.get(byte1) && dec.get(byte2)) {
            int diff = static_cast<unsigned char>(byte1) - static_cast<unsigned char>(byte2);
            mse += diff * diff;
            totalPixels++;
        }

        mse /= totalPixels;
        return (mse == 0) ? 100 : (20 * log10(255.0 / sqrt(mse)));
    }
};

#endif //COMPARE_H
