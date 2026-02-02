#include <iostream>
#include <sstream>

#include "tracelib/event.hpp"
#include "tracelib/perfFolded.hpp"


// Event
PerfFoldedEventData::PerfFoldedEventData(const long long int samples) : samples(samples) {
}


PerfFoldedEvent::PerfFoldedEvent(const Type type, const std::string &name,
                                 const std::string &processName,
                                 const int tid, const int pid, const int ppid,
                                 std::unique_ptr<PerfFoldedEventData> &&data) :
Event(type, name, processName, tid, pid, ppid), data(std::move(data)){

}

PerfFoldedEvent::~PerfFoldedEvent() {
}

PerfFoldedEventData *PerfFoldedEvent::getData() {
    return this->data.get();
}

std::string PerfFoldedEvent::toString() {
    std::stringstream sstream;
    sstream << Event::toString() << "\n"
            << this->data->samples;
    return sstream.str();
}

// Parser
PerfFoldedParser::PerfFoldedParser(const std::string &traceFilePath, const std::string& metadataFilePath,
                                   std::ifstream::pos_type startPos, std::ifstream::pos_type endPos)
                                   : Parser(traceFilePath, metadataFilePath, startPos, endPos) {
    if (this->charBeforeStart != '\n') {
        this->traceFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void PerfFoldedParser::parseMetadata() {
    Parser::parseMetadata();
}

std::unique_ptr<Event> PerfFoldedParser::getNextEvent() {
    // Check if we reached the end position
    if (this->endPos != std::ifstream::pos_type(-1)) {
        auto currentPos = traceFile.tellg();
        if (currentPos != std::ifstream::pos_type(-1) && endPos <= currentPos) {
            this->currentLine = "";
            return nullptr;
        }
    }

    // Retrieve a line from the file
    if (!std::getline(this->traceFile, this->currentLine)) {
        this->currentLine = "";
        return nullptr;
    }
    if (this->currentLine.empty() or this->currentLine.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
        return std::move(this->getNextEvent());
    }
    this->numberOfEvents++;

    // Parse the line
    size_t pos;
    if ( (pos = this->currentLine.find(' ')) == std::string::npos ) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    std::string stackSampleString = this->currentLine.substr(0, pos); // TODO stringviews here
    const std::string sampleCntString = this->currentLine.substr(pos+1);
    const long long int sampleCnt = std::stoll(sampleCntString);

    if ( (pos = stackSampleString.find(';')) == std::string::npos ) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    auto event = std::make_unique<PerfFoldedEvent>(Event::STACK_SAMPLE, "");
    event->data = std::make_unique<PerfFoldedEventData>(sampleCnt);

    event->processName = stackSampleString.substr(0, pos);
    stackSampleString = stackSampleString.substr(pos+1) + ';';

    event->pid = processNameToProcessIdMap.try_emplace(event->processName, processNameToProcessIdMap.size() + 1).first->second;

    size_t start = 0;
    size_t end = 0;
    while ( (end = stackSampleString.find(';', start)) != std::string::npos ) {
        event->stackSample.push_back(stackSampleString.substr(start, end-start));
        start = end + 1;
    }
    event->name = event->stackSample.back();

    return event;
}

// NodeData
void PerfFoldedNodeData::combine(std::unique_ptr<Event> &&enterEvent, std::unique_ptr<Event> &&exitEvent) {
    auto* event = static_cast<PerfFoldedEvent*>(enterEvent.get());
    const PerfFoldedEventData* data = event->getData();
    this->samplesCnt = data->samples;
}

long long int PerfFoldedNodeData::getDuration() const {
    // Note: Returns the invocation frequency since this information is not collected
    return this->samplesCnt;
}

long long int PerfFoldedNodeData::getInvocationFrequency() const {
    return this->samplesCnt;
}

void PerfFoldedNodeData::merge(const PerfFoldedNodeData &other) {
    this->samplesCnt += other.samplesCnt;
}

