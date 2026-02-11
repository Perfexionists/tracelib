#include <iostream>
#include <sstream>

#include "tracelib/perunSystemTap.hpp"


// Event
PerunSystemTapEventData::PerunSystemTapEventData(const long long int timestamp) :
timestamp(timestamp) {
}

PerunSystemTapEvent::PerunSystemTapEvent(const Type type,
                                         const std::string& name, const std::string& processName,
                                         const int tid, const int pid, const int ppid, PerunSystemTapEventData* data) :
Event(type, name, processName, tid, pid, ppid), data(data) {
}

PerunSystemTapEvent::~PerunSystemTapEvent() {
    delete this->data;
}

PerunSystemTapEventData* PerunSystemTapEvent::getData() {
    return this->data;
}

std::string PerunSystemTapEvent::toString() {
    std::stringstream sstream;
    sstream << "Event " << this->getEventTypeAsString() << "\n"
            << "  name: " << this->name << " " << "\n"
            << "  processName: " << this->processName<< " " << "\n"
            << "  tid, pid, ppid:" << this->tid << ", " << this->pid << ", " << this->ppid << "\n"
            << "  timestamp: " << this->getData()->timestamp;
    return sstream.str();
}


// Parser
PerunSystemTapParser::PerunSystemTapParser(const std::string& traceFilePath, const std::string& metadataFilePath):
Parser(traceFilePath, metadataFilePath) {
}

void PerunSystemTapParser::parseMetadata() {
    Parser::parseMetadata();
    if (this->metadataJson.is_object()) {
        this->metadata = this->metadataJson.get<Metadata>();
    }
}

std::unique_ptr<Event> PerunSystemTapParser::getNextEvent() {
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

    std::string values = currentLine.substr(0, pos);
    std::string name = currentLine.substr(pos+1);

    // Check if the name is actually an ID and retrive the name from metadata if possible
    bool isId = true;
    for (const auto& ch : name) {
        if (!std::isdigit(ch)) {
            isId = false;
            break;
        }
    }
    if (isId and this->metadataFile.is_open()) {
        // Convert the identifier to name based on metadata
        const int id = std::stoi(name);
        name = this->metadata.functions[id];
    }

    // Get type of the event
    pos = values.find(' ');
    if (pos == std::string::npos) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    const int perunEventType = std::stoi(values.substr(0, pos));
    values.erase(0, pos+1);

    // Parse the values based on the event type
    int tid, pid, ppid;
    tid = pid = ppid = -1;
    long long int timestamp = -1;
    int numOfScannedValues;
    PerunSystemTapEventData* data;
    std::unique_ptr<PerunSystemTapEvent> event;
    switch (perunEventType) {
        case PROCESS_BEGIN:
            numOfScannedValues = std::sscanf(values.c_str(), "%d %d %d %lld", &tid, &pid, &ppid, &timestamp);
            if (numOfScannedValues != 4) {
                std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
                exit(1);
            }
            data = new PerunSystemTapEventData(timestamp);
            event = std::make_unique<PerunSystemTapEvent>(Event::PROCESS_ENTER, name, name, tid, pid, ppid, data);
            break;
        case PROCESS_END:
            numOfScannedValues = std::sscanf(values.c_str(), "%d %d %d %lld", &tid, &pid, &ppid, &timestamp);
            if (numOfScannedValues != 4) {
                std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
                exit(1);
            }
            data = new PerunSystemTapEventData(timestamp);
            event = std::make_unique<PerunSystemTapEvent>(Event::PROCESS_EXIT, name, name, tid, pid, ppid, data);
            break;
        case THREAD_BEGIN:
            numOfScannedValues = std::sscanf(values.c_str(), "%d %d %lld", &tid, &pid, &timestamp);
            if (numOfScannedValues != 3) {
                std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
                exit(1);
            }
            data = new PerunSystemTapEventData(timestamp);
            event = std::make_unique<PerunSystemTapEvent>(Event::THREAD_ENTER, name, name, tid, pid, -1, data);
            break;
        case THREAD_END:
            numOfScannedValues = std::sscanf(values.c_str(), "%d %d %lld", &tid, &pid, &timestamp);
            if (numOfScannedValues != 3) {
                std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
                exit(1);
            }
            data = new PerunSystemTapEventData(timestamp);
            event = std::make_unique<PerunSystemTapEvent>(Event::THREAD_EXIT, name, name, tid, pid, -1, data);
            break;
        case USDT_BEGIN:
            numOfScannedValues = std::sscanf(values.c_str(), "%d %lld", &tid, &timestamp);
            if (numOfScannedValues != 2) {
                std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
                exit(1);
            }
            data = new PerunSystemTapEventData(timestamp);
            event = std::make_unique<PerunSystemTapEvent>(Event::USDT_ENTER, name, "", tid, -1, -1, data);
            break;
        case USDT_END:
            numOfScannedValues = std::sscanf(values.c_str(), "%d %lld", &tid, &timestamp);
            if (numOfScannedValues != 2) {
                std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
                exit(1);
            }
            data = new PerunSystemTapEventData(timestamp);
            event = std::make_unique<PerunSystemTapEvent>(Event::USDT_EXIT, name, "", tid, -1, -1, data);
            break;
        case FUNC_BEGIN:
            numOfScannedValues = std::sscanf(values.c_str(), "%d %lld", &tid, &timestamp);
            if (numOfScannedValues != 2) {
                std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
                exit(1);
            }
            data = new PerunSystemTapEventData(timestamp);
            event = std::make_unique<PerunSystemTapEvent>(Event::FUNCTION_ENTER, name, "", tid, -1, -1, data);
            this->numberOfFunctionCalls++;
            break;
        case FUNC_END:
            numOfScannedValues = std::sscanf(values.c_str(), "%d %lld", &tid, &timestamp);
            if (numOfScannedValues != 2) {
                std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
                exit(1);
            }
            data = new PerunSystemTapEventData(timestamp);
            event = std::make_unique<PerunSystemTapEvent>(Event::FUNCTION_EXIT, name, "", tid, -1, -1, data);
            break;
        case CORRUPT:
            // Skips the corrupt event
            return std::move(this->getNextEvent());
        default:
            std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
            exit(1);
    }

    return event;
}

void PerunSystemTapNodeData::combine(std::unique_ptr<Event> &&enterEvent, std::unique_ptr<Event> &&exitEvent) {
    const auto *enterData = static_cast<PerunSystemTapEventData *>(enterEvent->getData());
    const auto *exitData = static_cast<PerunSystemTapEventData *>(exitEvent->getData());

    const auto duration = exitData->timestamp - enterData->timestamp;
    this->durations.push_back(duration);
}

void from_json(const nlohmann::json &j, PerunSystemTapParser::Metadata &metadata) {
    if (j.contains("functions") and j["functions"].is_object()) {
        for (auto &[functionId, functionName]: j["functions"].items()) {
            int functionIdAsInt = std::stoi(functionId);
            metadata.functions[functionIdAsInt] = functionName.get<std::string>();
        }
    }
}

// NodeData
long long int PerunSystemTapNodeData::getDuration() const {
    long long int overallDruation = 0;
    for (auto& duration : this->durations) {
        overallDruation += duration;
    }
    return overallDruation;
}

long long int PerunSystemTapNodeData::getInvocationFrequency() const {
    return this->durations.size();
}
