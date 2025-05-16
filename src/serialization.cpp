#include "tracelib/serialization.hpp"


std::vector<char> Serialization::compress(const std::string &data, int compressionLevel) {
    size_t maxCompressedSize = ZSTD_compressBound(data.size());
    std::vector<char> compressedData(maxCompressedSize);

    // Compress the data
    size_t compressedSize = ZSTD_compress(
        compressedData.data(), // Destination buffer
        maxCompressedSize, // Destination buffer size
        data.data(), // Source buffer
        data.size(), // Source size
        compressionLevel // Compression level (1-22, higher = better compression but slower)
    );

    if (ZSTD_isError(compressedSize)) {
        std::cerr << "[E] Compression failed: " + std::string(ZSTD_getErrorName(compressedSize)) << std::endl;
    }

    // Resize the vector to actual compressed size
    compressedData.resize(compressedSize);
    std::cout << "Compressed data size: " << formatFileSize(compressedSize) << std::endl;
    std::cout << "Compression ratio: " << static_cast<double>(compressedSize) / static_cast<double>(data.size()) *
            100 << "%" << std::endl;

    return compressedData;
}

std::string Serialization::decompress(const std::vector<char> &compressedData) {
    unsigned long long originalSize = ZSTD_getFrameContentSize(compressedData.data(), compressedData.size());
    if (originalSize == ZSTD_CONTENTSIZE_UNKNOWN || originalSize == ZSTD_CONTENTSIZE_ERROR) {
        std::cerr << "[E]: Unknown or invalid compressed data size" << std::endl;
    }

    std::vector<char> decompressedData(originalSize);

    // Decompress the data
    size_t decompressedSize = ZSTD_decompress(
        decompressedData.data(), // Destination buffer
        originalSize, // Destination buffer size
        compressedData.data(), // Source buffer
        compressedData.size() // Source size
    );

    if (ZSTD_isError(decompressedSize)) {
        std::cerr << "[E]: Decompression error: " + std::string(ZSTD_getErrorName(decompressedSize)) << std::endl;
    }

    // Make sure we got the expected size
    if (decompressedSize != originalSize) {
        std::cerr << "[E]: Decompressed size doesn't match expected size" << std::endl;
    }

    return std::string{decompressedData.begin(), decompressedData.end()};
}
