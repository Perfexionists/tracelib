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
PerfFoldedParser::PerfFoldedParser(const std::string &traceFilePath, const std::string& metadataFilePath) : Parser(traceFilePath, metadataFilePath){
}

void PerfFoldedParser::parseMetadata() {
    Parser::parseMetadata();
}

std::unique_ptr<Event> PerfFoldedParser::getNextEvent() {
    // Retrieve a line from the file
    std::string currentLine;
    if (!std::getline(this->traceFile, currentLine)) {
        this->currentLine = "";
        return nullptr;
    }
    this->currentLine = currentLine;
    if (currentLine.empty() or currentLine.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
        return std::move(this->getNextEvent());
    }
    this->numberOfEvents++;

    // Parse the line
    size_t pos;
    if ( (pos = currentLine.find(' ')) == std::string::npos ) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    std::string stackSampleString = currentLine.substr(0, pos); // TODO stringviews here
    const std::string sampleCntString = currentLine.substr(pos+1);
    const long long int sampleCnt = std::stoll(sampleCntString);

    if ( (pos = stackSampleString.find(';')) == std::string::npos ) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    const std::string processName = stackSampleString.substr(0, pos);
    stackSampleString = stackSampleString.substr(pos+1) + ';';

    int processId = this->processIdCounter;
    if (auto it = this->processNameToProcessIdMap.find(processName); it != this->processNameToProcessIdMap.end()) {
        processId = it->second;
    } else {
        this->processNameToProcessIdMap.emplace(processName, this->processIdCounter++);
    }

    std::vector<std::string> stackSample;
    size_t start = 0;
    size_t end = 0;
    while ( (end = stackSampleString.find(';', start)) != std::string::npos ) {
        stackSample.push_back(stackSampleString.substr(start, end-start));
        start = end + 1;
    }

    auto event = std::make_unique<PerfFoldedEvent>(Event::STACK_SAMPLE, stackSample.back());
    event->stackSample = stackSample;
    event->processName = processName;
    event->pid = processId;
    event->data = std::make_unique<PerfFoldedEventData>(sampleCnt);
    return event;
}

// NodeData
void PerfFoldedNodeData::combine(Event *enterEvent, Event *exitEvent) {
    auto* event = static_cast<PerfFoldedEvent*>(enterEvent);
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

