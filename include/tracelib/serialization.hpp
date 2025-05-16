#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <zstd.h>
#include <filesystem>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "utils.hpp"

/**
 * @brief Serialization formats provided by different libraries and implementations.
 */
enum class SerializationFormat {
    BOOST_TEXT = 0,
    BOOST_BINARY = 1,
    PERF_FOLDED = 2,
};

/**
 * @brief A class implementing serialization with different libraries. Currently supports only Boos:serialization
 */
class Serialization {
public:
    /**
     * @brief Serialization formats supported by the Serialization class
     */
    enum class Format {
        BOOST_TEXT = 0,
        BOOST_BINARY = 1,
    };

    Serialization() = delete;
    ~Serialization() = delete;
    Serialization(const Serialization&) = delete;
    Serialization& operator=(const Serialization&) = delete;

    /**
     * @brief Form a string with specified object serialized in specified format.
     * @tparam T Type of the serialized object
     * @param obj object to serialize
     * @param format format to serialize into
     * @return serialized data in a string
     */
    template<typename T>
    static std::string serialize(T *obj, Format format = Format::BOOST_BINARY);

    /**
     * @brief From a string with serialized data in specified format deserialize the data into the specified object.
     * @tparam T Type of the object being deserialized
     * @param obj object to deserialize into
     * @param data serialized data
     * @param format format of the serialized data
     */
    template<typename T>
    static void deserialize(T *obj, const std::string &data, Format format = Format::BOOST_BINARY);

    /**
     * @brief Form a vector of characters that contains compressed data that were passed as a parameter.
     * @param data the data to compress
     * @param compressionLevel the compression level to compress at
     * @return the compressed data as vector of chars
     */
    static std::vector<char> compress(const std::string &data, int compressionLevel = 3);

    /**
     * @brief Decompress specified compressed data into a string.
     * @param compressedData compressed data to decompress
     * @return decompressed data
     */
    static std::string decompress(const std::vector<char> &compressedData);
};

template<typename T>
std::string Serialization::serialize(T *obj, Format format) {
    std::stringstream ss;
    switch (format) {
        case Format::BOOST_BINARY: {
            boost::archive::binary_oarchive oa(ss);
            oa << BOOST_SERIALIZATION_NVP(*obj);
            break;
        }
        case Format::BOOST_TEXT: {
            boost::archive::text_oarchive oa(ss);
            oa << BOOST_SERIALIZATION_NVP(*obj);
            break;
        }
        default:
            std::cerr << "[E]: Unimplemented serialization format!" << std::endl;
    }
    std::string output = ss.str();
    std::cout << "Serialized data size: " << formatFileSize(output.size()) << std::endl;

    return output;
}

template<typename T>
void Serialization::deserialize(T *obj, const std::string &data, Format format) {
    std::stringstream ss(data);

    switch (format) {
        case Format::BOOST_BINARY: {
            boost::archive::binary_iarchive ia(ss);
            ia >> (*obj);
            break;
        }
        case Format::BOOST_TEXT: {
            boost::archive::text_iarchive ia(ss);
            ia >> (*obj);
            break;
        }
        default:
            std::cerr << "[E]: Unimplemented deserialization format!" << std::endl;
    }
}


#endif //SERIALIZATION_HPP
