#ifndef PERUNPIN_HPP
#define PERUNPIN_HPP

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "nodeData.hpp"
#include "event.hpp"
#include "parser.hpp"

// Event
/**
 * @brief Specifies what is the event associated to. Either Routine/Function (RTN) or Basic block (BBL).
 */
enum PerunPinEventGranularity {
    RTN = 0,
    BBL = 1,
};

/**
 * @brief Specifies when in relation to the Routine or Basic block did the event happen. Either before or after.
 */
enum PerunPinEventLocation {
    BEFORE = 0,
    AFTER = 1,
};

/**
 * @brief Implementation of EventData class. Represents data collected by Perun's Pin tracer.
 */
class PerunPinEventData final : public EventData {
public:
    /**
     * @brief timestamp of the event
     */
    long long int timestamp;

    // Location
    /**
     * @brief Path to the source code file
     */
    std::string filePath;
    /**
     * @brief Source code lines associated with the event.
     */
    std::vector<int> lines;

    // Function Arguments
    /**
     * @brief Indices of collected arguments.
     */
    std::vector<int> argumentIndices;
    /**
     * @brief Values of collected arguments.
     */
    std::vector<long long> argumentValues;

    explicit PerunPinEventData(long long int timestamp);
};

/**
 * @brief Implementation of Event class. Represents events collected by Perun's Pin tracer.
 */
class PerunPinEvent final : public Event {
public:
    /**
     * @brief data collected by Perun's Pin tracer.
     */
    PerunPinEventData* data;
    /**
     * @brief Identifier for Basic blocks.
     */
    long long int id;

    PerunPinEvent(Type type,
                  const std::string& name, const std::string& processName = "",
                  int tid = -1, int pid = -1, int ppid = -1,
                  PerunPinEventData *data = nullptr);

    ~PerunPinEvent() override;

    /**
     * @brief Retrieves event data specific to format of data collected by Perun's Pin tracer stored in this event.
     * @return event data specific to Perun's Pin format
     */
    PerunPinEventData* getData() override;

    /**
     * @brief Forms a string representation of the event
     * @return string representation of the event
     */
    std::string toString() override;
};

// Parser
/**
 * @brief Implementation of the Parser class. This parser is able to convert Perun's Pin tracer format
 * into PerunPinEvents.
 */
class PerunPinParser final : public Parser {
public:
    /**
     * @brief Location in the source code.
     */
    struct Location {
        /**
         * @brief Source code file identifier.
         */
        int fileId;
        /**
         * @brief Lines in source code.
         */
        std::vector<int> lines;
    };

    /**
     * @brief Information about a function.
     */
    struct FunctionMetadata {
        /**
         * @brief The function name.
         */
        std::string name;
        /**
         * @brief Indices of collected arguments.
         */
        std::vector<int> argumentIndices;
        /**
         * @brief Location of the function in source code.
         */
        Location location;
    };

    /**
     * @brief Information about a basic block.
     */
    struct BasicBlockMetadata {
        /**
         * @brief The name of the function this basic block belongs to.
         */
        std::string functionName;
        /**
         * @brief The number of instructions within the basic block.
         */
        long long int instructionsCnt;
        /**
         * @brief The location of the basic block in the source code.
         */
        Location location;
    };

    /**
     * @brief Metadata provided for the Perun's Pin format
     */
    struct Metadata {
        /**
         * @brief Vector of source code files.
         * They are addressed by index into this vector everywhere else.
         */
        std::vector<std::string> filePaths;
        /**
         * @brief Map of function id to function metadata
         */
        std::map<long long int, FunctionMetadata> functions;
        /**
         * @brief Map of basic block id to basic block metadata
         */
        std::map<long long int, BasicBlockMetadata> basicBlocks;
    };

    /**
     * @brief metadata from the Perun's Pin tracer format
     */
    Metadata metadata;

    explicit PerunPinParser(const std::string &traceFilePath, const std::string &metadataFilePath = "");

    /**
     * @brief Parse metadata from json. The Perun's Pin tracer format expects metadata.
     * Will be executed by the builder before the parsing of the trace file begins.
     */
    void parseMetadata() override;

    /**
     * @brief Parse the event from Perun's Pin tracer format into the PerunPinEvent and return it.
     * @return PerunPinEvent instance with data corresponding to an event from Perun's Pin tracer format.
     * Caller is responsible for deleting the event.
     */
    std::unique_ptr<Event> getNextEvent() override;

    friend void from_json(const nlohmann::json& j, Location& location);
    friend void from_json(const nlohmann::json& j, FunctionMetadata& functionMetadata);
    friend void from_json(const nlohmann::json& j, BasicBlockMetadata& basicBlockMetadata);
    friend void from_json(const nlohmann::json& j, Metadata& metadata);
};

/**
 * @brief Implementation of the NodeData class. Used to store data within the CCT/CCG structures.
 */
class PerunPinNodeData final : public NodeData {
public:
    /**
     * @brief Durations of events calculated from their timestamps.
     */
    std::vector<long long int> durations = {};

    // Location in source code
    /**
     * @brief Path to the source file coresponding to the function.
     */
    std::string filePath = "";
    /**
     * @brief Lines in the source file corresponding to the function
     */
    std::vector<int> lines = {};

    // Arguments
    /**
     * @brief A map from argument index to its value.
     * Note: storing only integers since the Perun Pin implementation currently does support only
     * simple data types (float and double are not fully supported yet) and strings (char*)
     * are converte to length of the string an similarly characters are converted to their ordinal value.
     * index of the argument -> vector of values
     */
    std::unordered_map<int, std::vector<long long>> argumentValues;

    // Basic Blocks
    /**
     * @brief A map from basic block id to the duration spent inside the basic block.
     */
    std::unordered_map<long long int, long long int> basicBlockDurationsMap;

    PerunPinNodeData() = default;
    ~PerunPinNodeData() = default;

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
        ar & BOOST_SERIALIZATION_NVP(filePath);
        ar & BOOST_SERIALIZATION_NVP(lines);
        ar & BOOST_SERIALIZATION_NVP(argumentValues);
        ar & BOOST_SERIALIZATION_NVP(basicBlockDurationsMap);
    }
};
#endif //PERUNPIN_HPP
