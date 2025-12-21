#ifndef NODEDATA_HPP
#define NODEDATA_HPP

#include "event.hpp"

/**
 * @brief Base class for data stored in nodes of Calling Context Tree and Connected Call Graph.
 */
class NodeData {
public:
    NodeData() = default;
    virtual ~NodeData() = default;

    /**
     * @brief Combines data from enter end exit events and stores them in the derivedl class from this class. The
     * exit event is not mandatory because the event type might not be split into Exit and Enter.
     * @param enterEvent enter event associated with the node this data is stored in
     * @param exitEvent exit event associated with the node this data is stored in
     */
    virtual void combine(std::unique_ptr<Even> && enterEvent, std::unique_ptr<Even> && exitEvent = nullptr) = 0;

    /**
     * @brief Retrieve a duration metric for the node this data is stored in. Usually time spent in the function.
     * @return duration metric
     */
    virtual long long int getDuration() const = 0;
    /**
     * @brief Retrieve a invocation frequency metric for the node this data is stored in.
     * @return invocation frequency of the node
     */
    virtual long long int getInvocationFrequency() const = 0;
};

#endif //NODEDATA_HPP
