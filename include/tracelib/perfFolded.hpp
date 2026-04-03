#ifndef PERFFOLDED_HPP
#define PERFFOLDED_HPP

#include <boost/serialization/nvp.hpp>

#include <memory>
#include "nodeData.hpp"
#include "parser.hpp"
#include "event.hpp"
#include "utils.hpp"

// Event
/**
 * @brief Implementation of EventData class. This is used to store specific data from Perf Folded format.
 */
class PerfFoldedEventData final : public EventData {
public:
    /**
     * @brief number of times a sample was encountered
     */
    long long int samples;

    explicit PerfFoldedEventData(long long int samples);
};

/**
 * @brief Implementation of Event class. This is used for communication with builder of the CCT/CCG structure.
 */
class PerfFoldedEvent final : public Event {
public:
    std::unique_ptr<PerfFoldedEventData> data;

    PerfFoldedEvent(Type type, const std::string& name,
                    const std::string& processName = "",
                    int tid = -1, int pid = -1, int ppid = -1,
                    std::unique_ptr<PerfFoldedEventData> &&data = nullptr);

    ~PerfFoldedEvent() override;

    /**
     * @brief Retrieves event data specific to Perf Folded format stored in this event.
     * @return event data specific to Perf Folded format
     */
    PerfFoldedEventData* getData() override;

    /**
     * @brief Forms a string representation of the event
     * @return string representation of the event
     */
    std::string toString() override;
};

// Parser
/**
 * @brief Implementation of the Parser class. This parser is able to convert Perf Folded  format into PerfFoldedEvents.
 */
class PerfFoldedParser final : public Parser {
public:
    /**
     * @brief Helper map to associate the created PID with the provided process name from the Perf Folded format
     */
    std::unordered_map<std::string, int> processNameToProcessIdMap;

    explicit PerfFoldedParser (ParTraceHandle &handle,
                               std::ifstream::pos_type startPos = 0,
                               std::ifstream::pos_type endPos = std::ifstream::pos_type(-1));
    PerfFoldedParser (const PerfFoldedParser &) = delete;
    ~PerfFoldedParser() override;

    /**
     * @brief Parse metadata into json object. The perf folded format does not expect any metadata.
     * Will be executed by the builder before the parsing of the trace file begins.
     */
    void parseMetadata() override;

    /**
     * @brief Parse the event from Perf Folded format into the PerfFoldedEvent and return it.
     * @return PerfFoldedEvent instance with data corresponding to an event from Perf Folded Event.
     * Caller is responsible for deleting the event.
     */
    std::unique_ptr<Event> getNextEvent() override;

private:
    /*
     * @brief Read the input file until either of the characters from the string "; \n" or EOF are found.
     * @return A pair of the string_view, excluding the delimiter found (valid until the next call after this function returns the \n delim),
     * and the delimiter character found. On error, the delim is 0.
     */
    std::pair<std::string_view, char> readUntilDelim();

    char *traceFileCurrent = nullptr;
    char *traceFileEnd = nullptr;
    char *endPtr = nullptr;
};

// CCT node
/**
 * @brief Implementation of the NodeData class. Used to store data within the CCT/CCG structures.
 */
class PerfFoldedNodeData final : public NodeDataBase {
public:
    long long int samplesCnt = 0;

    PerfFoldedNodeData() = default;
    ~PerfFoldedNodeData() = default;

    /**
     * @brief Combines data from enter end exit events and stores them in this class. The
     * exit event is not mandatory because the event type might not be split into Exit and Enter.
     * User can define aggregation of the collected data here.
     * @param enterEvent enter event associated with the node this data is stored in
     * @param exitEvent exit event associated with the node this data is stored in
     */
    void combine(std::unique_ptr<Event> &&enterEvent, std::unique_ptr<Event> &&exitEvent = nullptr) override;

    /**
     * @brief Retrieve a duration metric for the node this data is stored in. Here it is the invocation count.
     * @return duration metric
     */
    long long int getDuration() const override;
    /**
     * @brief Retrieve an invocation frequency metric for the node this data is stored in. Here it is the invocation count.
     * @return invocation frequency of the node
     */
    long long int getInvocationFrequency() const override;

    void merge(PerfFoldedNodeData &&other);

private:
    friend class boost::serialization::access;

    /**
     * @brief Serialization and deserialization function for this class.
     * @tparam Archive type of the archive
     * @param ar the achive to serialize into or deserialize from
     * @param version the version of the serialization
     */
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_NVP(samplesCnt);
    }
};

#endif //PERFFOLDED_HPP
