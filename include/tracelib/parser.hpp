#ifndef PARSER_HPP
#define PARSER_HPP

#include <fstream>
#include <nlohmann/json.hpp>

#include "event.hpp"

/**
 * @brief Base class for parser implementation.
 */
class Parser {
protected:
    std::string traceFilePath;
    std::ifstream traceFile;
    std::string currentLine;

    std::string metadataFilePath;
    std::ifstream metadataFile;

    const std::ifstream::pos_type endPos;

public:
    nlohmann::json metadataJson{};

    long long numberOfEvents = 0;
    long long numberOfFunctionCalls = 0;

    explicit Parser(const std::string& traceFilePath, const std::string& metadataFilePath = "",
                    std::ifstream::pos_type startPos = 0,
                    std::ifstream::pos_type endPos = std::ifstream::pos_type(-1));
    virtual ~Parser();

    // Prevent duplication of the parser
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    /**
     * @brief This function preparses the metadata. It is meant to be used before the parsing starts to be used by the
     * event parsing.
     */
    virtual void parseMetadata();

    /**
     * @brief Parses next line from trace file and forms new event from it.
     * Caller is responsible for deleting the event.
     * @return an event parsed from the trace file and enriched by the metadata
     */
    virtual std::unique_ptr<Event> getNextEvent() = 0;

    /**
     * @brief Set new trace file path. The old file will be closed and newone will be used to create next events.
     * @param filePath new path to trace file
     */
    void setTraceFile(const std::string& filePath);

    /**
     * @brief Get file that is used for creation of events.
     * @return file path to current trace file
     */
    std::string getTraceFile();

    /**
     * @brief Set new metadata file path. The old file will be closed and new one will be used to parse metadata upon
     * call of parserMetadata method.
     * @param filePath new path to metadata file
     */
    void setMetadataFile(const std::string& filePath);
    /**
     * @brief Get file that is source of metadata.
     * @return file path to current metadata file
     */
    std::string getMetadataFile();

    /**
     * @brief Retrieve original line from trace file as a string that is corresponding to the last event created.
     * If getNextEvent was not called for the current trace file it is set to empty string.
     * @return original line from trace file as a string corresponding to the last event created
     */
    std::string getCurrentOriginalLine();
};

#endif // PARSER_HPP
