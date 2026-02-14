#ifndef EVENT_H
#define EVENT_H

#include <optional>
#include <string>
#include <vector>

class EventData {
};

/**
 * @brief A base class representing an event from a trace file.
 */
class Event {
public:
 /**
  * @brief Possible types of the event. Events specify multiple categories (Functions, Basic blocks, Process, ...) and
  * some of them are also devided into Enter and Exit events based on the location in relation to category.
  */
 enum Type {
        FUNCTION_ENTER = 0,
        FUNCTION_EXIT = 1,
        BASIC_BLOCK_ENTER = 2,
        BASIC_BLOCK_EXIT= 3,
        USDT_ENTER = 4,
        USDT_EXIT = 5,
        PROCESS_ENTER = 6,
        PROCESS_EXIT = 7,
        THREAD_ENTER = 8,
        THREAD_EXIT = 9,
        STACK_SAMPLE = 10,
        CUSTOM = 11,

        // Sentinel value
        EVENT_TYPE_SENTINEL_ = 12,
    };

    Type type = EVENT_TYPE_SENTINEL_;

    /**
     * @brief The name of the event. Usually the function name it is related to.
     * Note: the name attribute is treated as the identifier of nodes in the resulting CCT/CCG structure.
     * Therefore, for basic block it should be set to the function it coresponds to. USDT events are treated as
     * custom function like nodes (they are attached to the tree as if functions). For Stack sample the name should be
     * the last function on the stack. Thread and process events are used only to adjust the forest structure, therefore
     * their name is not set.
     */
    std::string name = "";

    /**
     * @brief Function names in the order as observed on stack sample. This is mandatory only if
     * the event type is STACK_SAMPLE.
     */
    std::vector<std::string_view> stackSample = {};
    /**
     * @brief Name of the process this event is associated with.
     */
    std::string processName = "";
    /**
     * @brief Thread identifier of the process this event is associated with.
     */
    int tid = -1;
    /**
     * @brief Process identifier of the process this event is associated with.
     */
    int pid = -1;
    /**
     * @brief Process identifier of the parent process this event is associated with.
     */
    int ppid = -1;

    /**
     * @brief Creates default empty event.
     */
    Event() = default;

    /**
     * @brief Creates event with specified parameters.
     * @param type the type of the event
     * @param name  the name of the event (usually the function name it is associated with)
     * @param processName the name of the process the event is associated with
     * @param tid the thread identifier of the process the event is associated with
     * @param pid the process identifier of the process the event is associated with
     * @param ppid the process identifier of the parent of the process the is associated with
     */
    Event(Type type, const std::string& name,
          const std::string& processName = "", int tid = -1, int pid = -1, int ppid = -1);

    /**
     * @brief Deletes the entire event.
     */
    virtual ~Event() = default;

    /**
     * @brief Retrieves the data stored in the event
     * This function is used to retrieve sepcial data from the event which are then combined from complementary events
     * into the node data of CCT/CCG node structure.
     * @return the data stored in event (defined by the derived event class)
     */
    virtual EventData* getData() = 0;

    /**
     * @brief Check if specified event is complementary to the current instance. Menaning if the event type is
     * the same category (Function, Basic block, Process, ...) but is the other location (Enter or Exit).
     * @param other the other event
     * @return true if the event is complementary to the other event, false otherwise
     */
    virtual bool isComplementaryEvent(const Event *other) const;

    /**
     * @brief calls isComplementaryEvent function
     * @param other the other event
     * @return true if events are complementary, false otherwise
     */
    bool operator==(const Event &other) const;

    /**
     * @brief Forms string representation of the event
     * @return string representation of the event
     */
    virtual std::string toString();

    /**
     * @brief Retrieves the complementary event type to the specified event type
     * @param eventType the event type to which the complementary event type needs to be retrieved
     * @return complementary event type
     */
    virtual std::optional<Type> getComplementaryEventType(Type eventType) const;

    /**
     * @brief Forms string from the type of current event instance
     * @return event type as a string
     */
    virtual std::string getEventTypeAsString() const;

    /**
     * @brief Checks if the current event is enter event type
     * @return true if current event is enter event, false otherwise
     */
    virtual bool isEnterEvent() const;
    /**
     * @brief Checks if the current event is exit event type
     * @return true if current event is exit event, false otherwise
     */
    virtual bool isExitEvent() const;


};


#endif //EVENT_H
