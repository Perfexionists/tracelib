#ifndef PERUNSYSTEMTAP_HPP
#define PERUNSYSTEMTAP_HPP

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

#include "nodeData.hpp"
#include "parser.hpp"


// Event

/**
 * @brief Internal event types for the parsing of the Perun's SystemTap tracer format
 */
enum PerunSystemTapEventType {
    FUNC_BEGIN = 0,
    FUNC_END = 1,
    USDT_SINGLE = 2,
    USDT_BEGIN = 3,
    USDT_END = 4,
    THREAD_BEGIN = 5,
    THREAD_END = 6,
    PROCESS_BEGIN = 7,
    PROCESS_END = 8,
    CORRUPT = 9,
};

/**
 * @brief Implementation of EventData class. Represents data collected by Perun's SystemTap tracer.
 */
class PerunSystemTapEventData final : public EventData {
public:
    /**
     * @brief timestamp of the event
     */
    long long int timestamp;

    explicit PerunSystemTapEventData(long long int timestamp);
    //~PerunSystemTapEventData() override = default;

};

/**
 * @brief Implementation of Event class. Represents events collected by Perun's Pin tracer.
 */
class PerunSystemTapEvent final : public Event {
public:

    /**
     * @brief data collected by Perun's SystemTap tracer.
     */
    PerunSystemTapEventData* data;

    PerunSystemTapEvent(Type type,
                        const std::string& name, const std::string& processName = "",
                        int tid = -1, int pid = -1, int ppid = -1,
                        PerunSystemTapEventData *data = nullptr);

    ~PerunSystemTapEvent() override;

    /**
     * @brief Retrieves event data specific to format of data collected by Perun's SystemTap tracer stored in this event.
     * @return event data specific to Perun's SystemTap format
     */
    PerunSystemTapEventData* getData() override;

    /**
     * @brief Forms a string representation of the event
     * @return string representation of the event
     */
    std::string toString() override;
};

// Parser
/**
 * @brief Implementation of the Parser class. This parser is able to convert Perun's SystemTap tracer format
 * into PerunSystemTapEvents.
 */
class PerunSystemTapParser final : public Parser {
public:
    /**
     * @brief Metadata provided for the Perun's SystemTap format
     */
    struct Metadata {
        /**
         * @brief Map from function ids to function names
         */
        std::map<int, std::string> functions;
    };

    /**
     * @brief metadata from the Perun's SystemTap tracer format
     */
    Metadata metadata;

    explicit PerunSystemTapParser(const std::string &traceFilePath, const std::string &metadataFilePath = "");

    /**
     * @brief Parse metadata from json. The Perun's SystemTap tracer format expects metadata.
     * Will be executed by the builder before the parsing of the trace file begins.
     */
    void parseMetadata() override;

    /**
     * @brief Parse the event from Perun's SystemTap tracer format into the PerunSystemTapEvent and return it.
     * @return PerunSystemTapEvent instance with data corresponding to an event from Perun's SystemTap tracer format.
     * Caller is responsible for deleting the event.
     */
    std::unique_ptr<Event> getNextEvent() override;

    friend void from_json(const nlohmann::json& j, Metadata& metadata);
};


/**
 * @brief Implementation of the NodeData class. Used to store data within the CCT/CCG structures.
 */
class PerunSystemTapNodeData final : public NodeDataBase {
public:
    /**
     * @brief Durations of events calculated from their timestamps.
     */
    std::vector<long long int> durations = {};

    PerunSystemTapNodeData() = default;
    ~PerunSystemTapNodeData() = default;

    /**
     * @brief Combines data from enter end exit events and stores them in this class. The
     * exit event is not mandatory because the event type might not be split into Exit and Enter.
     * User can define aggregation of the collected data here.
     * @param enterEvent enter event associated with the node this data is stored in
     * @param exitEvent exit event associated with the node this data is stored in
     */
    void combine(std::unique_ptr<Event> &&enterEvent, std::unique_ptr<Event> &&exitEvent) override;

    /**
     * @brief Retrieve a duration metric for the node this data is stored in.
     * Here it is the duration of the function in microseconds.
     * @return duration in microseconds
     */
    long long int getDuration() const override;

    /**
     * @brief Retrieve an invocation frequency metric for the node this data is stored in.
     * Here it is the invocation count of the function.
     * @return invocation frequency of the function
     */
    long long int getInvocationFrequency() const override;

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
        ar & BOOST_SERIALIZATION_NVP(durations);
    }
};

#endif //PERUNSYSTEMTAP_HPP
