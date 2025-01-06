//
// Created by weza on 1/6/25.
//

#ifndef COMPARE_H
#define COMPARE_H

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

        // Calculate compression ratio
        double compressionRatio = static_cast<double>(compressedSize) / originalSize;
        std::cout << "Original size: " << originalSize << " bytes" << std::endl;
        std::cout << "Compressed size: " << compressedSize << " bytes" << std::endl;
        std::cout << "Compression ratio: " << compressionRatio << std::endl;
        std::cout << "Bit error rate: " << static_cast<double>(differentBits) / totalBits << std::endl;
    }
};

#endif //COMPARE_H
