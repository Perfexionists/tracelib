#include <iostream>
#include <string>

#include "tracelib/parser.hpp"

Parser::Parser(const std::string& traceFilePath, const std::string& metadataFilePath) {
    this->traceFilePath = traceFilePath;
    this->currentLine = "";

    this->metadataFilePath = metadataFilePath;
    if (!this->metadataFilePath.empty()) {
        this->metadataFile.open(this->metadataFilePath);
        if (!this->metadataFile.is_open()) {
            std::cerr << "[W]: Couldn't open metadata file: " << this->metadataFilePath << "!" << std::endl;
        }
    }

    this->traceFile.open(this->traceFilePath);
    if (!this->traceFile.is_open()) {
        std::cerr << "[E]: Couldn't open file " << this->traceFilePath << "!" << std::endl;
    }
}

Parser::~Parser() {
    this->traceFile.close();
    this->metadataFile.close();
}

void Parser::parseMetadata() {
    if (!this->metadataFile.is_open()) {
        return;
    }
    try {
        this->metadataJson = nlohmann::json::parse(this->metadataFile);
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "[E]: JSON parse error: " << e.what() << std::endl;
    } catch (nlohmann::json::type_error& e) {
        std::cerr << "[E]: JSON type error: " << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cerr << "[E]: Exception: " << e.what() << std::endl;
    }
}

void Parser::setTraceFile(const std::string& filePath) {
    this->traceFile.close();
    this->traceFilePath = filePath;
    this->currentLine = "";

    this->traceFile.open(this->traceFilePath);
    if (!this->traceFile.is_open()) {
        std::cerr << "[E]: Couldn't open file " << this->traceFilePath << "!" << std::endl;
    }
}

std::string Parser::getTraceFile() {
    return this->traceFilePath;
}

void Parser::setMetadataFile(const std::string& filePath) {
    this->metadataFile.close();
    this->metadataFilePath = filePath;

    if (!this->metadataFilePath.empty()) {
        this->metadataFile.open(this->metadataFilePath);
        if (!this->metadataFile.is_open()) {
            std::cerr << "[W]: Couldn't open metadata file: " << this->metadataFilePath << "!" << std::endl;
        }
    }
}

std::string Parser::getMetadataFile() {
    return this->metadataFilePath;
}

std::string Parser::getCurrentOriginalLine() {
    return this->currentLine;
}
