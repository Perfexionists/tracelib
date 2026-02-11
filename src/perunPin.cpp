#include <iostream>
#include <sstream>

#include "tracelib/perunPin.hpp"

#define EXPECTED_NUMBER_OF_VALUES 4


// Event
PerunPinEventData::PerunPinEventData(const long long int timestamp) : timestamp(timestamp) {
}

PerunPinEvent::PerunPinEvent(Type type, const std::string &name,
                             const std::string &processName,
                             int tid, int pid, int ppid,
                             PerunPinEventData *data) : Event(type, name, processName, tid, pid, ppid), data(data){
}

PerunPinEvent::~PerunPinEvent() {
    delete this->data;
}

PerunPinEventData* PerunPinEvent::getData() {
    return this->data;
}

std::string PerunPinEvent::toString() {
    std::stringstream sstream;
    sstream << Event::toString() << "\n"
            << this->data->timestamp;
    return sstream.str();
}


// Parser
PerunPinParser::PerunPinParser(const std::string &traceFilePath, const std::string& metadataFilePath) :
Parser(traceFilePath, metadataFilePath) {
}

void PerunPinParser::parseMetadata() {
    Parser::parseMetadata();
    if (this->metadataJson.is_object()) {
        this->metadata = this->metadataJson.get<Metadata>();
        this->metadataJson.clear();
    }
}

std::unique_ptr<Event> PerunPinParser::getNextEvent() {

    // Retrieve a line from the file
    std::string currentLine;
    if (!std::getline(this->traceFile, currentLine)) {
        this->currentLine = nullptr;
        return nullptr;
    }
    this->currentLine = &currentLine;
    if (currentLine.empty() or currentLine.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
        return std::move(this->getNextEvent());
    }
    this->numberOfEvents++;

    // Parse the line
    size_t pos;
    if ( (pos = currentLine.find(';')) == std::string::npos ) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }

    // Parse flags that specify the granularity and location of the traced event
    // Note: Granularity specifies the primitive that is traced - basic block or routine/function. Location,
    // on the other hand, specifies where was the primitive instrumented - before or after the traced event happened.
    std::string flags = currentLine.substr(0, pos);
    std::string valuesString = currentLine.substr(pos+1) + ';';

    const auto granularity = static_cast<PerunPinEventGranularity>(flags[0] - '0');
    const auto location = static_cast<PerunPinEventLocation>(flags[1] - '0');

    // Parse values from the event (id, tid, pid and timestamp)
    size_t start = 0;
    size_t end = 0;
    std::vector<std::string> values = {};
    while ( (end = valuesString.find(';', start)) != std::string::npos ) {
        values.push_back(valuesString.substr(start, end-start));
        start = end + 1;
    }

    const long long int id = stoll(values[0]);
    const int tid = stoi(values[1]);
    const int pid = stoi(values[2]);
    const long long int timestamp = stoll(values[3]);



    Event::Type eventType;
    std::string functionName; // TODO stringview?
    std::string sourceCodeFilePath;
    std::vector<int> sourceCodeLines;
    auto* data = new PerunPinEventData(timestamp);
    if (granularity == RTN) {
        const auto functionsMetadataIterator = this->metadata.functions.find(id);
        if (functionsMetadataIterator != this->metadata.functions.end()) {
            functionName = functionsMetadataIterator->second.name;
            data->lines = functionsMetadataIterator->second.location.lines;
            data->filePath = this->metadata.filePaths[functionsMetadataIterator->second.location.fileId];
        } else {
            // Note: Using id instead of name. Expecting incomplete metadata instead of malformed trace file.
            functionName = values[0];
            std::cerr << "[W]: Found a function id (" << values[0] << ") that is not present in the metadata!" << std::endl;
        }
        if (location == BEFORE) {
            eventType = Event::FUNCTION_ENTER;
            if (values.size() > EXPECTED_NUMBER_OF_VALUES and // There are aditional values
                functionsMetadataIterator != this->metadata.functions.end()) { // and function id is in the metadata
                // Treat them as function arguments
                // Note: if the function is not in metadata these arguments will be ignored since the metadata info
                // is crucial for storing them.
                std::vector<long long> argumentValues;
                for (size_t i = EXPECTED_NUMBER_OF_VALUES; i < values.size(); ++i) {
                    bool digitsOnly = true;
                    for (char c: values[i]) {
                        if (!std::isdigit(c)) {
                            digitsOnly = false;
                            break;
                        }
                    }
                    if (values[i] == "") {
                        digitsOnly = false;
                    }
                    if (digitsOnly) {
                        argumentValues.push_back(stoll(values[i]));
                    } else {
                        argumentValues.push_back(values[i].size());
                    }
                }
                data->argumentIndices = functionsMetadataIterator->second.argumentIndices;
                data->argumentValues = argumentValues;
            }
            this->numberOfFunctionCalls++;
        } else if (location == AFTER) {
            eventType = Event::FUNCTION_EXIT;
        }
    } else if (granularity == BBL) {
        const auto basicBlocksMetadataIterator = this->metadata.basicBlocks.find(id);
        if (basicBlocksMetadataIterator != this->metadata.basicBlocks.end()) {
            functionName = basicBlocksMetadataIterator->second.functionName;
            data->lines = basicBlocksMetadataIterator->second.location.lines;
            data->filePath = this->metadata.filePaths[basicBlocksMetadataIterator->second.location.fileId];
        } else {
            // Note: Using id instead of name. Expecting incomplete metadata instead of malformed trace file.
            functionName = values[0];
            std::cerr << "[W]: Found a basic block id (" << values[0] << ") that is not present in the metadata!" << std::endl;
        }
        if (location == BEFORE) {
            eventType = Event::BASIC_BLOCK_ENTER;
        } else if (location == AFTER) {
            eventType = Event::BASIC_BLOCK_EXIT;
        }
    } else {
        std::cerr << "[E]: Unimplemented granularity!" << std::endl;
        exit(1);
    }

    auto event = std::make_unique<PerunPinEvent>(eventType, functionName, "", tid, pid, -1, data);
    event->id = id;
    return std::move(event);
}

void from_json(const nlohmann::json &j, PerunPinParser::Location &location) {
    j.at("fileId").get_to(location.fileId);
    if (j.contains("lines") and j["lines"].is_array()) {
        j.at("lines").get_to(location.lines);
    }
}

void from_json(const nlohmann::json &j, PerunPinParser::FunctionMetadata &functionMetadata) {
    j.at("name").get_to(functionMetadata.name);
    j.at("location").get_to(functionMetadata.location);
    if (j.contains("argumentIndices") && j["argumentIndices"].is_array()) {
        j.at("argumentIndices").get_to(functionMetadata.argumentIndices);
    }
}

void from_json(const nlohmann::json &j, PerunPinParser::BasicBlockMetadata &basicBlockMetadata) {
    j.at("functionName").get_to(basicBlockMetadata.functionName);
    j.at("instructionsCnt").get_to(basicBlockMetadata.instructionsCnt);
    j.at("location").get_to(basicBlockMetadata.location);
}

void from_json(const nlohmann::json &j, PerunPinParser::Metadata &metadata) {
    if (j.contains("filePaths") && j["filePaths"].is_array()) {
        j.at("filePaths").get_to(metadata.filePaths);
    }
    if (j.contains("functions") && j["functions"].is_object()) {
        for (auto &[functionId, functionMetadataJson]: j["functions"].items()) {
            long long functionIdAsInt = std::stoll(functionId);
            metadata.functions[functionIdAsInt] = functionMetadataJson.get<PerunPinParser::FunctionMetadata>();
        }
    }
    if (j.contains("basicBlocks") && j["basicBlocks"].is_object()) {
        for (auto &[basicBlockId, basicBlockMetadataJson]: j["basicBlocks"].items()) {
            long long basicBlockIdAsInt = std::stoll(basicBlockId);
            metadata.basicBlocks[basicBlockIdAsInt] = basicBlockMetadataJson.get<PerunPinParser::BasicBlockMetadata>();
        }
    }
}

// NodeData
void PerunPinNodeData::combine(std::unique_ptr<Event> &&enterEvent, std::unique_ptr<Event> &&exitEvent) {
    auto* pinEnterEvent = static_cast<PerunPinEvent*>(enterEvent.get());
    auto* pinExitEvent = static_cast<PerunPinEvent*>(exitEvent.get());
    const auto* enterData = pinEnterEvent->getData();
    const auto* exitData = pinExitEvent->getData();

    const auto duration = exitData->timestamp - enterData->timestamp;
    if (pinEnterEvent->type == Event::FUNCTION_ENTER) {
        this->durations.push_back(duration);
        for (size_t i = 0; i < enterData->argumentIndices.size(); ++i) {
            const int argumentIndex = enterData->argumentIndices[i];
            const int argumentValue = enterData->argumentValues[i];
            this->argumentValues[argumentIndex].push_back(argumentValue);
        }
        if (this->filePath.empty()) {
            this->filePath = enterData->filePath;
        }
        if (this->lines.empty()) {
            this->lines = enterData->lines;
        }
    } else if (pinEnterEvent->type == Event::BASIC_BLOCK_ENTER) {
        auto iterator = this->basicBlockDurationsMap.find(pinEnterEvent->id);
        if (iterator == this->basicBlockDurationsMap.end()) {
            this->basicBlockDurationsMap.emplace(pinEnterEvent->id, duration);
        } else {
            iterator->second += duration;
        }
    }
    this->durations.push_back(duration);
}

long long int PerunPinNodeData::getDuration() const {
    long long int overallDruation = 0;
    for (auto& duration : this->durations) {
        overallDruation += duration;
    }
    return overallDruation;
}

long long int PerunPinNodeData::getInvocationFrequency() const {
    return this->durations.size();
}

