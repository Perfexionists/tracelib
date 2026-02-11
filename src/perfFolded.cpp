#include <iostream>
#include <sstream>
#include <charconv>

#include "tracelib/event.hpp"
#include "tracelib/perfFolded.hpp"


// Event
PerfFoldedEventData::PerfFoldedEventData(const long long int samples) : samples(samples) {
}


PerfFoldedEvent::PerfFoldedEvent(const Type type, std::string &&line, const std::string &name,
                                 const std::string &processName,
                                 const int tid, const int pid, const int ppid,
                                 std::unique_ptr<PerfFoldedEventData> &&data) :
Event(type, name, processName, tid, pid, ppid), line(std::move(line)), data(std::forward<std::unique_ptr<PerfFoldedEventData>>(data)){

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
        if (endPos <= currentPos && currentPos != std::ifstream::pos_type(-1)) {
            this->currentLine = nullptr;
            return nullptr;
        }
    }

    // Retrieve a line from the file
    std::string currentLine;
    if (!std::getline(this->traceFile, currentLine)) {
        this->currentLine = nullptr;
        return nullptr;
    }
    this->currentLine = &currentLine;
    if (currentLine.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
        return std::forward<std::unique_ptr<Event>>(this->getNextEvent());
    }
    ++this->numberOfEvents;

    // Parse the line
    size_t pos;
    if ( (pos = currentLine.find(' ')) == std::string::npos ) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    unsigned long long sampleCnt;
    auto res = std::from_chars(currentLine.data() + pos + 1,
                               currentLine.data() + currentLine.size(),
                               sampleCnt);
    if (res.ec != std::errc{} || res.ptr != currentLine.data() + currentLine.size()) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    std::string stackSampleString = currentLine.substr(0, pos); // TODO stringviews here

    if ( (pos = stackSampleString.find(';')) == std::string::npos ) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    auto event = std::make_unique<PerfFoldedEvent>(Event::STACK_SAMPLE, std::move(currentLine), "");
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

void PerfFoldedNodeData::merge(std::unique_ptr<PerfFoldedNodeData> &&other) {
    this->samplesCnt += other->samplesCnt;
}

