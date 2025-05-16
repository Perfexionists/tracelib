#include <vector>
#include <iostream>
#include <sstream>

#include "tracelib/event.hpp"


Event::Event(const Type type, const std::string& name, const std::string& processName,
             const int tid, const int pid, const int ppid) :
type(type), name(name), processName(processName), tid(tid), pid(pid), ppid(ppid) {
}

bool Event::isComplementaryEvent(const Event* other) const {
    if (this->getComplementaryEventType(this->type) == other->type) {
       return this->processName == other->processName and
              this->name == other->name and
              this->tid == other->tid and
              this->pid == other->pid and
              this->ppid == other->ppid;
    }
    return false;
}

bool Event::operator==(const Event& other) const {
   return this->isComplementaryEvent(&other);
}

std::string Event::toString() {
    std::stringstream sstream;
    sstream << "Event " << this->getEventTypeAsString() << "\n"
            << "  name: " << this->name << " " << "\n"
            << "  processName: " << this->processName<< " " << "\n"
            << "  tid, pid, ppid:" << this->tid << ", " << this->pid << ", " << this->ppid;
    return sstream.str();
}

EventData* Event::getData() {
    return nullptr;
}

std::optional<Event::Type> Event::getComplementaryEventType(const Type eventType) const  {
    switch (eventType) {
        case EVENT_TYPE_SENTINEL_: return std::nullopt; // Should never be accessed
        case FUNCTION_ENTER: return FUNCTION_EXIT;
        case FUNCTION_EXIT: return FUNCTION_ENTER;
        case BASIC_BLOCK_ENTER: return BASIC_BLOCK_EXIT;
        case BASIC_BLOCK_EXIT: return BASIC_BLOCK_ENTER;
        case USDT_ENTER: return USDT_EXIT;
        case USDT_EXIT: return USDT_ENTER;
        case PROCESS_ENTER: return PROCESS_EXIT;
        case PROCESS_EXIT: return PROCESS_ENTER;
        case THREAD_ENTER: return THREAD_EXIT;
        case THREAD_EXIT: return THREAD_ENTER;
        case STACK_SAMPLE: return std::nullopt;
        case CUSTOM: return std::nullopt;
    }
    return std::nullopt;
}

std::string Event::getEventTypeAsString() const {
    if (this->type >= Event::EVENT_TYPE_SENTINEL_) {
        std::cerr << "[W]: Found undefined event type in base Event class!\n";
        return "";
    }

    const std::vector<std::string> eventTypeToStringMap = {
        "FUNCTION_ENTER", "FUNCTION_EXIT",
        "BASIC_BLOCK_ENTER", "BASIC_BLOCK_EXIT",
        "USDT_ENTER", "USDT_EXIT",
        "PROCESS_ENTER", "PROCESS_EXIT",
        "THREAD_ENTER", "THREAD_EXIT",
        "STACK_SAMPLE", "CUSTOM"
    };
    return eventTypeToStringMap[this->type];
}

bool Event::isEnterEvent() const {
    switch (this->type) {
        case FUNCTION_ENTER:
        case BASIC_BLOCK_ENTER:
        case USDT_ENTER:
        case PROCESS_ENTER:
        case THREAD_ENTER:
        case STACK_SAMPLE:
            return true;
        default:
            return false;
    }
}

bool Event::isExitEvent() const {
    switch (this->type) {
        case FUNCTION_EXIT:
        case BASIC_BLOCK_EXIT:
        case USDT_EXIT:
        case PROCESS_EXIT:
        case THREAD_EXIT:
            return true;
        default:
            return false;
    }
}
