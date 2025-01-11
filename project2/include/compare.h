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
};

#endif //COMPARE_H
