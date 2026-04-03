#include <iostream>
#include <sstream>
#include <charconv>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "tracelib/event.hpp"
#include "tracelib/perfFolded.hpp"


// Event
PerfFoldedEventData::PerfFoldedEventData(const long long int samples) : samples(samples) {
}


PerfFoldedEvent::PerfFoldedEvent(const Type type, const std::string &name,
                                 const std::string &processName,
                                 const int tid, const int pid, const int ppid,
                                 std::unique_ptr<PerfFoldedEventData> &&data) :
Event(type, name, processName, tid, pid, ppid), data(std::forward<std::unique_ptr<PerfFoldedEventData>>(data)){

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
PerfFoldedParser::PerfFoldedParser(ParTraceHandle &handle,
                                   std::ifstream::pos_type startPos, std::ifstream::pos_type endPos)
                                   : Parser("", "", startPos, endPos) {
    this->traceFileEnd = handle.getDataPtr() + handle.getFileSize();
    this->endPtr = handle.getDataPtr() + endPos;
    this->traceFileCurrent = handle.getDataPtr() + startPos;
    if (startPos != 0 && this->traceFileCurrent[-1] != '\n') {
        while (this->readUntilDelim().second != '\n') {}
    }
}

PerfFoldedParser::~PerfFoldedParser() {
}

void PerfFoldedParser::parseMetadata() {
    Parser::parseMetadata();
}

std::pair<std::string_view, char> PerfFoldedParser::readUntilDelim() {
    const char *start = this->traceFileCurrent;

    std::string_view::size_type length = 0;
    while (this->traceFileCurrent < this->traceFileEnd) {
        char current = *(this->traceFileCurrent++);
        switch (current) {
            case ';': // Fall through
            case ' ': // Fall through
            case '\n':
                ++this->traceFileCurrent;
                return std::pair{ std::string_view{ start, length }, current };
            default:
                break; // Do nothing
        }
        ++length;
    }
    return std::pair{ std::string_view{}, '\0' };
}

std::unique_ptr<Event> PerfFoldedParser::getNextEvent() {
    // Check if we reached the end position.
    if (this->endPtr <= this->traceFileCurrent && this->endPtr != nullptr) {
        return nullptr;
    }

    // TODO This implementation forbids spaces in function names.
    auto event = std::make_unique<PerfFoldedEvent>(Event::STACK_SAMPLE, "");

    bool expecting_function = true;
    bool first_function = true;
    do {
        auto [sv, delim] = this->readUntilDelim();
        if (delim == ' ') {
            expecting_function = false;
        } else if (delim != ';') {
            if (delim == '\n') {
                return std::forward<std::unique_ptr<Event>>(this->getNextEvent());
            } else if (delim == EOF && sv.empty()) {
                return nullptr;
            }
            std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
            exit(1);
        }

        if (first_function) {
            event->processName = std::move(sv);
            event->pid = processNameToProcessIdMap.try_emplace(event->processName, processNameToProcessIdMap.size() + 1).first->second;
            first_function = false;
        } else {
            event->stackSample.emplace_back(std::move(sv));
        }
    } while (expecting_function);
    if (event->stackSample.empty()) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }
    event->name = event->stackSample.back();

    auto [sv, delim] = this->readUntilDelim();
    if (delim != '\n' && delim != EOF) { // TODO This makes DOS newline invalid
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }

    unsigned long long sampleCnt;
    auto res = std::from_chars(sv.data(), sv.data() + sv.size(), sampleCnt);
    if (res.ec != std::errc{} || res.ptr != sv.data() + sv.size()) {
        std::cerr << "[E]: Unexpected format of trace file!" << std::endl;
        exit(1);
    }

    event->data = std::make_unique<PerfFoldedEventData>(sampleCnt);
    ++this->numberOfEvents;
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

void PerfFoldedNodeData::merge(PerfFoldedNodeData &&other) {
    this->samplesCnt += other.samplesCnt;
}

