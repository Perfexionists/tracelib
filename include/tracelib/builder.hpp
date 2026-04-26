#ifndef BUILDER_HPP
#define BUILDER_HPP

#include "parser.hpp"
#include "perfFolded.hpp"
#include "event.hpp"
#include "callingContextTree.hpp"
#include "callGraph.hpp"
#include "utils.hpp"
#include <system_error>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

// Helper specialization trait
template<template<class> class TemplatedType, class TemplateArgumentType>
struct IsSpecializationOf : std::false_type{};

template<template<class> class TemplatedType, class TemplateArgumentType>
struct IsSpecializationOf<TemplatedType, TemplatedType<TemplateArgumentType>> : std::true_type{};

// Helper specialization concepts
template<class T>
concept IsSpecializedSimpleGraphType = IsSpecializationOf<CCGraph, T>::value or
                                       IsSpecializationOf<CCTree, T>::value;

template<class T>
concept IsSpecializedContainerGraphType = IsSpecializationOf<DCGraph, T>::value or
                                          IsSpecializationOf<CCForest, T>::value;

template<class T>
concept IsSpecializedGraphType = IsSpecializationOf<CCGraph, T>::value or
                                 IsSpecializationOf<CCTree, T>::value or
                                 IsSpecializationOf<DCGraph, T>::value or
                                 IsSpecializationOf<CCForest, T>::value;

template<IsSpecializedGraphType Graph>
class EventProcessor {
private:
    using NodeData = typename Graph::valueType;
    /**
     * @brief Backlog of function enter events that need to be paired with exit events. Basically simulates
     * the call stack
     */
    std::vector<std::unique_ptr<Event>> functionsBacklog{};
    /**
     * @brief Backlog of basic block enter events that need to be paired with exit events.
     */
    std::vector<std::unique_ptr<Event>> basicBlocksBacklog{};
    /**
     * @brief stores paths for Connected Call Graphs. For every CCG mapped by pid and tid pair. It is used to
     * be able to get to the parent of current node.
     */
    std::unordered_map<std::pair<int, int>, std::vector<CCGNode<NodeData>*>, PairHash> currentPaths{};
    /**
     * @brief maps thread identifier to process identifier. It is used to adjust the event pid based on its tid. It is
     * necessary because the process and thread events might be mapping tid to pid and omitting the pid in other events.
     */
    std::unordered_map<int, int> tidToProcessMap{};


    std::vector<std::string_view> prevStackSample{};
    std::vector<nodeIdType> prevNodeIds{};

    // Helper values to normalize pid and tid in events to the value of first event
    // necessary for building singular tree from traces that contain multiple tids and pids
    /**
     * @brief If the event processed is the first one this will be true, otherwise false
     */
    bool isFirstEvent = true;
    /**
     * @brief the process name of the first event
     */
    std::string processName = "";
    /**
     * brief the process identifier of the first event
     */
    int pid = -1;
    /**
     * @brief the thread identifier of the first event
     */
    int tid = -1;

public:
    /**
     * @brief Creates a default event processor.
     */
    EventProcessor() = default;

    /**
     * @brief Deletes the event processor.
     */
    virtual ~EventProcessor();

    /**
     * @brief Process the information from specified event into the specified graph.
     * @tparam SimpleGraph Either the Calling Context Tree or Connected Call Graph
     * @param graph the graph to reflect the event information in
     * @param event the event with information that's to be processed into a graph
     */
    template<IsSpecializedSimpleGraphType SimpleGraph>
    void processEvent(SimpleGraph* graph, std::unique_ptr<Event> &&event);

    /**
     * @brief Process the information from specified event into the specified graph.
     * @tparam ContainerGraph Either the Calling Context Forest (Contains multiple CCTs) or Disconnected Call Graph (Contains multiple CCGs)
     * @param graph the graph to reflect the event information in
     * @param event the event with information that's to be processed into a graph
     */
    template<IsSpecializedContainerGraphType ContainerGraph>
    void processEvent(ContainerGraph* graph, std::unique_ptr<Event> &&event);

    /**
     * @brief a method that iterates over skipped events in the function/basic block backlog and calls handleSkippedEvent
     * method on those events. This enables user to specify what will be done with the events and analyze why they were
     * skipped.
     */
    virtual void processSkippedEvents();

    /**
     * @brief cleans up internal structures. Call this if you want to repurpose the EventProcessor on a different event
     * set that is not related to the previouse one.
     */
    virtual void cleanUpHelperStructures();

protected:
    /**
     * @brief Handles a function enter event by storing the information in specified calling context tree.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree to store the information from event in
     * @param event the event to handle
     */
    virtual void handleFunctionEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a function enter event by storing the information in specified connected call graph.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph to store the information from event in
     * @param event the event to handle
     */
    virtual void handleFunctionEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a function exit event by storing the information in specified calling context tree.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree to store the information from event in
     * @param event the event to handle
     */
    virtual void handleFunctionExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a function exit event by storing the information in specified connected call graph.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph to store the information from event in
     * @param event the event to handle
     */
    virtual void handleFunctionExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);

    /**
     * @brief Handles a basic block enter event by storing the information in specified CCT.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleBasicBlockEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a basic block enter event by storing the information in specified CCG.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleBasicBlockEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a basic block exit event by storing the information in specified CCT.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleBasicBlockExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a basic block exit event by storing the information in specified CCG.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleBasicBlockExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);

    /**
     * @brief Handles a USDT enter event by storing the information in specified CCT. The USDT events are handled as if
     * function events by default.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleUSDTEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a USDT enter event by storing the information in specified CCG. The USDT events are handled as if
     * function events by default.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleUSDTEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a USDT exit event by storing the information in specified CCT. The USDT events are handled as if
     * function events by default.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleUSDTExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a USDT exit event by storing the information in specified CCG. The USDT events are handled as if
     * function events by default.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleUSDTExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);

    /**
     * @brief Handles a process enter event by storing the information in specified CCT. These events are usually
     * desired to be present when multiple processes and threads are traced. By default they only maintain the
     * internal map of process and thread identifiers.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleProcessEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a process enter event by storing the information in specified CCG. These events are usually
     * desired to be present when multiple processes and threads are traced. By default they only maintain the
     * internal map of process and thread identifiers.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleProcessEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a process exit event by storing the information in specified CCT. These events are usually
     * desired to be present when multiple processes and threads are traced. By default they only maintain the
     * internal map of process and thread identifiers.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleProcessExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a process exit event by storing the information in specified CCG. These events are usually
     * desired to be present when multiple processes and threads are traced. By default they only maintain the
     * internal map of process and thread identifiers.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleProcessExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);

    /**
     * @brief Handles a thread enter event by storing the information in specified CCT. These events are usually
     * desired to be present when multiple processes and threads are traced. By default they only maintain the
     * internal map of process and thread identifiers.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleThreadEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a thread enter event by storing the information in specified CCG. These events are usually
     * desired to be present when multiple processes and threads are traced. By default they only maintain the
     * internal map of process and thread identifiers.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleThreadEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a thread exit event by storing the information in specified CCT. These events are usually
     * desired to be present when multiple processes and threads are traced. By default they only maintain the
     * internal map of process and thread identifiers.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleThreadExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a thread exit event by storing the information in specified CCG. These events are usually
     * desired to be present when multiple processes and threads are traced. By default they only maintain the
     * internal map of process and thread identifiers.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleThreadExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);

    /**
     * @brief Handles a stack sample event by storing the information in specified CCT.
     * This event does not have designated enter and exit, it is treated as enter event though.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleStackSampleEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a stack sample event by storing the information in specified CCG.
     * This event does not have designated enter and exit, it is treated as enter event though.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleStackSampleEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);

    /**
     * @brief Handles a custom event. By defualt this function just skips the event.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param tree calling context tree that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleCustomEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event);
    /**
     * @brief Handles a custom event. By defualt this function just skips the event.
     * It is meant to be overriden by the user if custom functionality is desired. The Disconnected Call Graph and
     * Calling Context Forest work internally with the CCT and CCG, thus these methods are called for the sub-graphs
     * (or trees). Hence, overriding this method for the CCG or CCT is enough.
     * @param graph connected call graph that should be updated with the information from specified event
     * @param event the event to be handled
     */
    virtual void handleCustomEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event);

    /**
     * @brief An event handler that is called for every unprocessed event. It is called from processSkippedEvents.
     * It is meant to be overriden with cusom logic by the user.
     * @param event the event that was not processed before
     */
    virtual void handleSkippedEvent(std::unique_ptr<Event> &&event);

    /**
     * @brief This function calls appropriate event handler for specified event and graph.
     * @tparam SimpleGraph the Connected Call Graph or the Calling Context Tree types
     * @param graph the graph to store information from the event into
     * @param event the event with new information for the specified graph
     */
    template<IsSpecializedSimpleGraphType SimpleGraph>
    void callEventHandler(SimpleGraph* graph, std::unique_ptr<Event> &&event);
};


template<IsSpecializedGraphType Graph>
EventProcessor<Graph>::~EventProcessor() {
    this->cleanUpHelperStructures();
}

template<IsSpecializedGraphType Graph>
template<IsSpecializedSimpleGraphType SimpleGraph>
void EventProcessor<Graph>::processEvent(SimpleGraph *graph, std::unique_ptr<Event> &&event) {
    // Normalize event pid and tid to the first event since we are building a tree or a graph from this function
    // and we don't expect the pid and tid to change
    if (this->isFirstEvent) {
        this->pid = event->pid;
        this->tid = event->tid;
        this->processName = event->processName;
        graph->processName = event->processName;
        graph->pid = event->pid;
        graph->tid = event->tid;
        if constexpr (IsSpecializationOf<CCGraph, SimpleGraph>::value) {
            // Note: only if we are building Call graph it is necessary to initialize one path a tree does not require this
            this->currentPaths.emplace(std::make_pair(event->pid, event->tid), std::vector<CCGNode<NodeData>*>{graph->getCurrentNode()});
        }
        this->isFirstEvent = false;
    } else {
        event->pid = this->pid;
        event->tid = this->tid;
    }
    this->callEventHandler(graph, std::forward<std::unique_ptr<Event>>(event));
}

template<IsSpecializedGraphType Graph>
template<IsSpecializedContainerGraphType ContainerGraph>
void EventProcessor<Graph>::processEvent(ContainerGraph* graph, std::unique_ptr<Event> &&event) {
    int pid = event->pid;
    int tid = event->tid;

    // adjust the tid and pid according to the helper map
    // Note: this is needed because the PROCESS_* and THREAD_* events announce
    // new thread and process id and the other events after that don't need to
    // specify PID in some formats. This way the process id gets mapped for the
    // event that does not specify it if there already was such thread id.
    if (pid < 0) {
        const auto it = this->tidToProcessMap.find(tid);
        if (it != this->tidToProcessMap.end()) {
            pid = it->second;
        }
    } else if (tid < 0) {
        // the main thread of the process tends to have same tid as pid
        const auto it = this->tidToProcessMap.find(pid);
        if (it != this->tidToProcessMap.end()) {
            tid = pid;
        }
    }
    if constexpr (IsSpecializationOf<CCForest, ContainerGraph>::value) {
        auto* currentTree = graph->getTree(pid, tid);// Searching for the adjusted pid and tid
        if (currentTree == nullptr and event->isEnterEvent() and event->type != Event::BASIC_BLOCK_ENTER) {
            // Create the tree if the event is in a new tid pid space, but exclude ending events since those can't create
            // a new tree.
            // Note: basic blocks currently don't require creation of a new tree since their data are always stored
            // in an existing function node.
            this->tidToProcessMap.emplace(event->tid, event->pid);  // Emplacing the original values
            currentTree = graph->addNewTree(event->pid, event->tid, event->processName);
        }
        this->callEventHandler(currentTree, std::forward<std::unique_ptr<Event>>(event));
    } else if constexpr (IsSpecializationOf<DCGraph, ContainerGraph>::value) {
        auto* currentGraph = graph->getGraph(pid, tid); // Searching for the adjusted pid and tid
        if (currentGraph == nullptr and event->isEnterEvent() and event->type != Event::BASIC_BLOCK_ENTER) {
            currentGraph = graph->addNewGraph(event->pid, event->tid, event->processName);
            // initialize the path for the graph
            this->tidToProcessMap.emplace(event->tid, event->pid); // Emplacing the original values
            if (event->type != Event::STACK_SAMPLE) {
                // When going through stack samples the pathing is irelevant since the whole path will be constructed every time
                this->currentPaths.emplace(std::make_pair(event->pid, event->tid), std::vector<CCGNode<NodeData>*>{currentGraph->getCurrentNode()});
            }
        }
        this->callEventHandler(currentGraph, std::forward<std::unique_ptr<Event>>(event));
        //Note: event handlers are responsible for updating current paths if exiting a function
    }
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::cleanUpHelperStructures() {
    this->tidToProcessMap.clear();
    this->currentPaths.clear(); // Note: don't need to delete the nodes - the graph structure is responsible for them
    this->isFirstEvent = true;
    this->pid = -1;
    this->tid = -1;
    this->processName = "";
}

template<IsSpecializedGraphType Graph>
template<IsSpecializedSimpleGraphType SimpleGraph>
void EventProcessor<Graph>::callEventHandler(SimpleGraph *graph, std::unique_ptr<Event> &&event) {
     switch (event->type) {
        case Event::FUNCTION_ENTER:
            this->handleFunctionEnterEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::FUNCTION_EXIT:
            this->handleFunctionExitEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::BASIC_BLOCK_ENTER:
            this->handleBasicBlockEnterEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::BASIC_BLOCK_EXIT:
            this->handleBasicBlockExitEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::USDT_ENTER:
            this->handleUSDTEnterEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::USDT_EXIT:
            this->handleUSDTExitEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::PROCESS_ENTER:
            this->handleProcessEnterEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::PROCESS_EXIT:
            this->handleProcessExitEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::THREAD_ENTER:
            this->handleThreadEnterEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::THREAD_EXIT:
            this->handleThreadExitEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::STACK_SAMPLE:
            this->handleStackSampleEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        case Event::CUSTOM:
            this->handleCustomEvent(graph, std::forward<std::unique_ptr<Event>>(event));
            break;
        default:
            this->handleSkippedEvent(std::forward<std::unique_ptr<Event>>(event));
            break;
    }
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleFunctionEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    if (tree == nullptr) {
        // Note: This should not happen. When building a Tree it is defined by the user and caught in
        // build function and when creating a Forest the tree is always created before this function
        // gets called.
        std::cerr << "[E]: Expected a coresponding tree for the event.\n";
        event = nullptr;
        return;
    }

    const auto &name = event->name;
    // Store the function enter event until coresponding function exit is found
    // and the data within the event can be combined and stored properly in a node.
    this->functionsBacklog.emplace_back(std::forward<std::unique_ptr<Event>>(event));

    // Search for the called function in the children of
    // the function node that represents the caller.
    auto [fName, fId] = tree->functionNameToIdInsert(name);
    auto [nodeId, _] = tree->tryEmplaceChild(tree->getCurrentNodeId(), fId, fName);
    tree->setCurrentNodeId(nodeId);
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleFunctionEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    const auto &name = event->name;
    // Store the function enter until coresponding function exit is found
    // and the data within the event can be combined and stored properly in a node.
    this->functionsBacklog.emplace_back(std::forward<std::unique_ptr<Event>>(event));

    // Search for the called function in the children (adjacent nodes) of the node that represents the caller.
    auto* child = graph->getChildOfCurrentNode(name);
    if (child == nullptr) {
        // Function was not called from this caller yet. Search all the nodes
        // and if node representing this function exists (if no create it)
        // and add it to the children of current node
        child = graph->getNode(name);
        if (child == nullptr) {
            child = graph->addNewNode(name);
        }
        graph->addChildToCurrentNode(child);
    }
    graph->setCurrentNode(child);
    this->currentPaths[std::make_pair(graph->pid, graph->tid)].push_back(child);
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleFunctionExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    if (tree == nullptr) {
        // Unexpected event without a tree
        std::cerr << "[W]: Skipping an event "
                << "(" << event->getEventTypeAsString() << ") "
                << "that should and does not have a tree!" << std::endl;
        event = nullptr;
        return;
    }

    bool foundTheEnterEventInBacklog = false;
    for (auto it = this->functionsBacklog.rbegin(); it != this->functionsBacklog.rend(); ++it) {
        auto *backloggedEnterEvent = it->get();
        if (event->isComplementaryEvent(backloggedEnterEvent)) {
            foundTheEnterEventInBacklog = true;
            tree->getNodeDataRef(tree->getCurrentNodeId()).combine(std::forward<std::unique_ptr<Event>>(*it), std::forward<std::unique_ptr<Event>>(event));
            this->functionsBacklog.erase((it + 1).base());
            break;
        }
    }
    if (!foundTheEnterEventInBacklog) {
        std::cerr << "[W]: Could not find funcion entering event at function exit."
                "Could not update the data in node!" << std::endl;
    }
    tree->setCurrentNodeId(tree->getNodeParent(tree->getCurrentNodeId()));
    event = nullptr;
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleFunctionExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    if (graph == nullptr) {
        // Unexpected event without a graph
        std::cerr << "[W]: Skipping an event "
                << "(" << event->getEventTypeAsString() << ") "
                << "that should and does not have a graph!" << std::endl;
        event = nullptr;
        return;
    }
    bool foundTheEnterEventInBacklog = false;
    for (auto it = this->functionsBacklog.rbegin(); it != this->functionsBacklog.rend(); ++it) {
        auto *backloggedEnterEvent = it->get();
        if (event->isComplementaryEvent(backloggedEnterEvent)) {
            foundTheEnterEventInBacklog = true;
            graph->getCurrentNode()->data->combine(std::forward<std::unique_ptr<Event>>(*it), std::forward<std::unique_ptr<Event>>(event));
            this->functionsBacklog.erase((it + 1).base());
            break;
        }
    }
    if (!foundTheEnterEventInBacklog) {
        std::cerr << "[W]: Could not find funcion entering event at function exit."
                "Could not update the data in node!" << std::endl;
    }
    auto &currentPath = this->currentPaths[std::make_pair(graph->pid, graph->tid)];
    currentPath.pop_back();

    auto *parent = currentPath[currentPath.size() - 1];
    graph->setCurrentNode(parent);
    event = nullptr;
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleBasicBlockEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    if (tree == nullptr) {
        // Unexpected event without a tree
        std::cerr << "[W]: Skipping an event "
                << "(" << event->getEventTypeAsString() << ") "
                << "that should and does not have a tree!" << std::endl;
        event = nullptr;
        return;
    }
    // Store the event in backlog and wait for its coresponding closing event to process it
    this->basicBlocksBacklog.emplace_back(std::forward<std::unique_ptr<Event>>(event));
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleBasicBlockEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    if (graph == nullptr) {
        // Unexpected event without a tree
        std::cerr << "[W]: Skipping an event "
                << "(" << event->getEventTypeAsString() << ") "
                << "that should and does not have a graph!" << std::endl;
        event = nullptr;
        return;
    }

    this->basicBlocksBacklog.emplace_back(std::forward<std::unique_ptr<Event>>(event));
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleBasicBlockExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    if (tree == nullptr) {
        // Unexpected event without a tree
        std::cerr << "[W]: Skipping an event "
                << "(" << event->getEventTypeAsString() << ") "
                << "that should and does not have a tree!" << std::endl;
        event = nullptr;
        return;
    }
    bool foundTheEnterEventInBacklog = false;
    for (auto it = this->basicBlocksBacklog.rbegin(); it != this->basicBlocksBacklog.rend(); ++it) {
        auto *backloggedEnterEvent = it->get();
        if (event->isComplementaryEvent(backloggedEnterEvent)) {
            foundTheEnterEventInBacklog = true;
            auto nodeToUpdate = tree->getCurrentNodeId();

            if (tree->getNodeFunctionName(nodeToUpdate) != event->name) {
                // The function exited sooner than the last basic block
                // TODO: this won't handle recursive calls
                // needs to check also if the last event before this was function exit
                auto [fId, wasFound] = tree->functionNameToId(event->name);
                if (wasFound) {
                    nodeToUpdate = tree->getNodeChild(nodeToUpdate, fId);
                }
                // Note: If node was not found even with adjustment. It is likely that the function was not recognized at the RTN
                // granularity and was not gathered. Thus, this basic block does not have a parent function and is skipped.
            }
            if (nodeToUpdate != NULL_NODE_ID) {
                tree->getNodeDataRef(nodeToUpdate).combine(std::forward<std::unique_ptr<Event>>(*it), std::forward<std::unique_ptr<Event>>(event));
            }
            this->basicBlocksBacklog.erase((it + 1).base());
            break;
        }
    }
    if (!foundTheEnterEventInBacklog) {
        std::cerr << "[W]: Could not find basic block entering event "
                  <<  "(" <<  event->name << ")" << " at basic block exit."
                  << "Could not update the data in node!" << std::endl;
    }
    event = nullptr;
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleBasicBlockExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    if (graph == nullptr) {
        // Unexpected event without a tree
        std::cerr << "[W]: Skipping an event "
                << "(" << event->getEventTypeAsString() << ") "
                << "that should and does not have a graph!" << std::endl;
        event = nullptr;
        return;
    }

    bool foundTheEnterEventInBacklog = false;
    for (auto it = this->basicBlocksBacklog.rbegin(); it != this->basicBlocksBacklog.rend(); ++it) {
        auto *backloggedEnterEvent = it->get();
        if (event->isComplementaryEvent(backloggedEnterEvent)) {
            foundTheEnterEventInBacklog = true;
            auto *nodeToUpdate = graph->getCurrentNode();
            if (nodeToUpdate->functionName != event->name) {
                // The function exited sooner than the last basic block (specific to Pin output)
                nodeToUpdate = graph->getChildOfCurrentNode(event->name);
            }
            if (nodeToUpdate) {
                nodeToUpdate->data->combine(std::forward<std::unique_ptr<Event>>(*it), std::forward<std::unique_ptr<Event>>(event));
            }
            this->basicBlocksBacklog.erase((it + 1).base());
            break;
        }
    }
    if (!foundTheEnterEventInBacklog) {
        std::cerr << "[W]: Could not find basic block entering event "
                  <<  "(" <<  event->name << ")" << " at basic block exit."
                  << "Could not update the data in node!" << std::endl;
    }
    event = nullptr;
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleUSDTEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    // For now treated as a custom function
    // TODO: decide if the usdts should have separate backlog
    // TODO: decide if a separate tree/forest should be created for USDTs
    this->handleFunctionEnterEvent(tree, std::forward<std::unique_ptr<Event>>(event));
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleUSDTEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    this->handleFunctionEnterEvent(graph, std::forward<std::unique_ptr<Event>>(event));
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleUSDTExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    // For now treated as a custom function
    // TODO: decide if the usdts should have separate backlog
    // TODO: decide if a separate tree/forest should be created for USDTs
    this->handleFunctionExitEvent(tree, std::forward<std::unique_ptr<Event>>(event));
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleUSDTExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
   this->handleFunctionExitEvent(graph, std::forward<std::unique_ptr<Event>>(event));
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleProcessEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    // Note: When building singular tree Process enter event is ignored
    // Note: The Process enter event is supposed to create a new tree for the process, however
    // creation of a tree for the event is already handled in the processEvent function and thus
    // this method just deletes the event.
    event = nullptr;
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleProcessEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    // Note: When building singular tree Process enter event is ignored
    // Note: The Process enter event is supposed to create a new graph for the process, however
    // creation of a graph for the event is already handled in the processEvent function and thus
    // this method just deletes the event.
    event = nullptr;
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleProcessExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    // Note: erasing the tid when building just a tree is not necessary, however calling erase
    // does nothing for empty map, thus it does not have to be checked.
    this->tidToProcessMap.erase(tree->tid);
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleProcessExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    // Note: erasing the tid when building just a graph is not necessary, however calling erase
    // does nothing for empty map, thus it does not have to be checked.
    this->tidToProcessMap.erase(graph->tid);
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleThreadEnterEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    // Note: When building singular tree Thread enter event is ignored
    // Note: The Thread enter event is supposed to create a new tree for the thread, however
    // creation of a tree for the event is already handled in the processEvent function and thus
    // this method just deletes the event.
    event = nullptr;
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleThreadEnterEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    // Note: When building singular graph Thread enter event is ignored
    // Note: The Thread enter event is supposed to create a new graph for the thread, however
    // creation of a graph for the event is already handled in the processEvent function and thus
    // this method just deletes the event.
    event = nullptr;
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleThreadExitEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    // Note: erasing the tid when building just a tree is not necessary, however calling erase
    // does nothing for empty map, thus it does not have to be checked.
    this->tidToProcessMap.erase(tree->tid);
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleThreadExitEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    // Note: erasing the tid when building just a graph is not necessary, however calling erase
    // does nothing for empty map, thus it does not have to be checked.
    this->tidToProcessMap.erase(graph->tid);
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleStackSampleEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    tree->setCurrentNodeId(AUXILIARY_ROOT_NODE_ID);

    size_t i = 0;
    bool foundDiff = false;
    this->prevNodeIds.resize(event->stackSample.size());
    for (auto functionName: event->stackSample) {
        nodeIdType nodeId;
        if (foundDiff || i >= this->prevStackSample.size() || functionName != this->prevStackSample[i]) {
            auto [fName, fId] = tree->functionNameToIdInsert(functionName);
            auto [nodeId, _] = tree->tryEmplaceChild(tree->getCurrentNodeId(), fId, fName);
            this->prevNodeIds[i] = std::move(nodeId);
            foundDiff = true;
        }
        tree->setCurrentNodeId(this->prevNodeIds[i]);
        ++i;
    }

    this->prevStackSample = std::move(event->stackSample);
    tree->getNodeDataRef(tree->getCurrentNodeId()).data.combine(std::forward<std::unique_ptr<Event>>(event), nullptr);
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleStackSampleEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    graph->setCurrentNode(graph->getRootNode());
    for (auto functionName: event->stackSample) {
        auto* child = graph->getChildOfCurrentNode(std::string(functionName)); // TODO Unnecessary string_view -> string
        if (child == nullptr) {
            // Function was not called from this caller yet. Search all the nodes
            // and if node representing this function exists (if it does not, create it)
            // and add it to the children of current node
            child = graph->getNode(std::string(functionName)); // TODO Unnecessary string_view -> string
            if (child == nullptr) { // Node with this name does not exist yet
                child = graph->addNewNode(std::string(functionName)); // TODO Unnecessary string_view -> string
            }
            child = graph->addChildToCurrentNode(child);
        }
        graph->setCurrentNode(child);
    }
    graph->getCurrentNode()->data->combine(std::forward<std::unique_ptr<Event>>(event), nullptr);
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleCustomEvent(CCTree<NodeData> *tree, std::unique_ptr<Event> &&event) {
    // Note: User can reimplement this function in deriving class to handle any custom defined events.
    // All of the events with type that is not defined by default go through here
    // It is also adwised to reimplement the isClosingEvent and isStartingEvent functions of the event
    // to make sure a tree in forest or a graph in multigraph gets created if not found for starting events
    // If the event is not devided into starting and closing events, define it as a starting event.

    // Default implementation only deletes the event.
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleCustomEvent(CCGraph<NodeData> *graph, std::unique_ptr<Event> &&event) {
    // Note: User can reimplement this function in deriving class to handle any custom defined events.
    // All of the events with type that is not defined by default go through here
    // It is also adwised to reimplement the isClosingEvent and isStartingEvent functions of the event
    // to make sure a tree in forest or a graph in multigraph gets created if not found for starting events
    // If the event is not devided into starting and closing events, define it as a starting event.

    // Default implementation only deletes the event.
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::handleSkippedEvent(std::unique_ptr<Event> &&event) {
    // Note: User can reimplement this function in deriving class to algorithmically go
    // through all missmatched events in backlogs.
    // Default implementation only deletes the event.
}

template<IsSpecializedGraphType Graph>
void EventProcessor<Graph>::processSkippedEvents() {
    for (auto& event : this->functionsBacklog) {
        this->handleSkippedEvent(std::forward<std::unique_ptr<Event>>(event));
    }
    this->functionsBacklog.clear();

    for (auto& event : this->basicBlocksBacklog) {
        this->handleSkippedEvent(std::forward<std::unique_ptr<Event>>(event));
    }
    this->basicBlocksBacklog.clear();
}

/**
 * @brief A class that is responsible for building and serializing graphs from traces.
 * @tparam Graph one of supported graph types:
 *                  Calling Context Tree (CCTree), Calling Context Forest (CCForest),
 *                  Connected Call Graph (CCGraph), or Disconnected Call Graph (DCGraph)
 */
template<IsSpecializedGraphType Graph>
class Builder {
protected:
    using NodeData = typename Graph::valueType;
    /**
     * @brief Event processor that is called to process events from the parser and builds the structure based on them.
     */
    std::unique_ptr<EventProcessor<Graph>> eventProcessor;

public:
    /**
     * @brief Creates the builder object with default or specified event processor that is responsible for handling events
     * from parsers of trace files.
     * @param eventProcessor a custom event processor can be specified to alter how structures is built
     */
    explicit Builder(std::unique_ptr<EventProcessor<Graph>> &&eventProcessor = nullptr);
    /**
     * @brief Deletes the whole builder.
     */
    virtual ~Builder();

    /**
     * @brief Builds the specified structure based on the events produced by provided parser.
     * @param graph one of the supported graph structures (CCTree, CCForest, CCGraph, or DCGraph)
     * @param parser a parser for the desired trace format
     */
    void build(Graph* graph, Parser* parser);

    /**
     * @brief Updates the specified structure based on the provided event. The graph should not be changed manually
     * before all the events are processed.
     * @param graph one of the supported graph structures (CCTree, CCForest, CCGraph, or DCGraph)
     * @param event an event that should be used to update the specified graph
     */
    void processEvent(Graph* graph, std::unique_ptr<Event> &&event);

    /**
     * @brief Serializes the specified graph structure (CCTree, CCFores, CCGraph, or DCGraph) into specified format and
     * compresses it as well. If compression level is 0 no compression is utilized. If compression level between 1-22
     * included is specified the serialized data is compressed and the file will have ".zst" file extension appended to
     * its name.
     * @param graph one of the supported graph structures (CCTree, CCForest, CCGraph, or DCGraph)
     * @param filePath path to the file where should the serialization be stored
     * @param format format of serialization
     * @param compressionLevel level of compression [0-22] (recommended default 3)
     */
    void serialize(Graph* graph, const std::string& filePath,
                   SerializationFormat format = SerializationFormat::BOOST_BINARY, int compressionLevel = 0);

    /**
     * @brief Deserializes a supported graph structure from given file path into the provided compatible graph structure.
     * @param graph one of the supported graph structures (CCTree, CCForest, CCGraph, or DCGraph) to deserialize into
     * @param filePath path to the file from where should the deserialization take the serialized data
     * @param format format of deserialization
     * @param decompress if true the decompression is executed before deserialization
     */
    void deserialize(Graph* graph, const std::string& filePath,
                     SerializationFormat format = SerializationFormat::BOOST_BINARY, bool decompress = false);

private:
    /**
     * @brief Helper function for serializing trees into Perf Folded format.
     * @param outputStream output stream where to store serialized data
     * @param treeToSerialize the CCTree to serialize
     */
    static void serializeCCTreeToPerfFoldedFormat(std::ostream &outputStream, CCTree<NodeData>* treeToSerialize);
};

template<IsSpecializedGraphType Graph>
Builder<Graph>::Builder(std::unique_ptr<EventProcessor<Graph>> &&eventProcessor) {
    if (eventProcessor == nullptr) {
        eventProcessor = std::make_unique<EventProcessor<Graph>>();
    }
    this->eventProcessor = std::move(eventProcessor);
    eventProcessor = nullptr;
}

template<IsSpecializedGraphType Graph>
Builder<Graph>::~Builder() {
}

template<IsSpecializedGraphType Graph>
void Builder<Graph>::build(Graph *graph, Parser *parser) {
    if (parser == nullptr or graph == nullptr) {
        std::cerr << "[E]: Building of structure unsuccessfull - wrong arguments!\n";
        return;
    }

    // parse the metadata before going through the trace itself
    parser->parseMetadata();

    std::unique_ptr<Event> currentEvent = parser->getNextEvent();
    while ((currentEvent = parser->getNextEvent()) != nullptr) {
        this->eventProcessor->processEvent(graph, std::move(currentEvent));
    }

    this->eventProcessor->processSkippedEvents();
    this->eventProcessor->cleanUpHelperStructures();
}

template<IsSpecializedGraphType Graph>
void Builder<Graph>::processEvent(Graph *graph, std::unique_ptr<Event> &&event) {
    if (graph == nullptr or event == nullptr) {
        return;
    }
    this->eventProcessor->processEvent(graph, std::forward<std::unique_ptr<Event>>(event));
}

template<IsSpecializedGraphType Graph>
void Builder<Graph>::serialize(Graph *graph, const std::string &filePath,
    SerializationFormat format, int compressionLevel) {

    std::string serialized;
    switch (format) {
        case SerializationFormat::BOOST_BINARY: {
            serialized = Serialization::serialize(graph, Serialization::Format::BOOST_BINARY);
            break;
        }
        case SerializationFormat::BOOST_TEXT: {
            serialized = Serialization::serialize(graph, Serialization::Format::BOOST_TEXT);
            break;
        }
        case SerializationFormat::PERF_FOLDED: {
            if constexpr (IsSpecializationOf<CCTree, Graph>::value) {
                std::stringstream sstream;
                Builder<Graph>::serializeCCTreeToPerfFoldedFormat(sstream, graph);
                serialized = sstream.str();
                break;
            } else if constexpr (IsSpecializationOf<CCForest, Graph>::value) {
                std::stringstream sstream;
                for (auto& [key, tree] : *graph) {
                    Builder<Graph>::serializeCCTreeToPerfFoldedFormat(sstream, tree);
                }
                serialized = sstream.str();
                break;
            } else {
                std::cerr << "[E]: Unimplemented type of serialization!" << std::endl;
                return;
            }
        }
        default: {
            std::cerr << "[E]: Unimplemented type of serialization!" << std::endl;
            return;
        }
    }

    if (compressionLevel > 0 and compressionLevel <= 22) {
        std::ofstream ofs(filePath + ".zst"); // TODO: error handling
        auto compressed = Serialization::compress(serialized, compressionLevel);
        ofs.write(compressed.data(), compressed.size());
    } else {
        std::ofstream ofs(filePath); // TODO: error handling
        ofs.write(serialized.data(), serialized.size());
    }
}

template<IsSpecializedGraphType Graph>
void Builder<Graph>::deserialize(Graph* graph, const std::string &filePath,
    SerializationFormat format, bool decompress) {

    std::ifstream ifs(filePath); // TODO: Error handling
    std::string serialized;

    if (decompress) {
        std::vector<char> compressed{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        std::cout << "Compressed data size: " << formatFileSize(compressed.size()) << std::endl;
        serialized = Serialization::decompress(compressed);
        std::cout << "Decompressed data size: " << formatFileSize(serialized.size()) << std::endl;

    } else {
        if (format != SerializationFormat::PERF_FOLDED) {
            // the PERF_FOLDED will read straight from file
            std::ostringstream sstream;
            sstream << ifs.rdbuf();
            serialized = sstream.str();
        }
    }

    switch (format) {
        case SerializationFormat::BOOST_BINARY: {
            Serialization::deserialize(graph, serialized, Serialization::Format::BOOST_BINARY);
            break;
        }
        case SerializationFormat::BOOST_TEXT: {
            Serialization::deserialize(graph, serialized, Serialization::Format::BOOST_TEXT);
            break;
        }
        case SerializationFormat::PERF_FOLDED: {
            // Note: deserialization is enabled only if the builder has NodeData = PerfFoldedNodeData
            // This means that if you serialized any other format into perf folded format
            // you need to create a new builder with NodeData = PerfFoldedNodeData to deserialize it or build it from the
            // serialized file.
            if constexpr (std::is_same_v<Graph, CCTree<PerfFoldedNodeData>> or
                          std::is_same_v<Graph, CCForest<PerfFoldedNodeData>>) {
                if (decompress) {
                    std::string tmpFoldFilePath = filePath + "-tmp.fold";
                    std::ofstream ofs(tmpFoldFilePath); // TODO: Error handling
                    ofs.write(serialized.data(), serialized.size());
                    ofs.close();
                    PerfFoldedParser parser(tmpFoldFilePath);
                    this->build(graph, &parser);
                    std::filesystem::remove(tmpFoldFilePath);
                } else {
                    PerfFoldedParser parser(filePath);
                    this->build(graph, &parser);
                }
                break;
            } else {
                std::cerr << "[E]: Unimplemented type of serialization!" << std::endl;
                return;
            }
        }
        default: {
            std::cerr << "[E]: Unimplemented type of serialization!" << std::endl;
        }
    }
}

template<IsSpecializedGraphType Graph>
void Builder<Graph>::serializeCCTreeToPerfFoldedFormat(std::ostream &outputStream, CCTree<NodeData> *treeToSerialize) {
    auto& tree = *treeToSerialize;
    for (auto [nodeId, nodePtr] : tree) {
        const long long int duration = nodePtr->data.getDuration();
        if (duration <= 0) {
            continue;
        }
        std::stack<std::string> stack;
        for (auto it = tree.pathToRootBegin(nodeId); it != tree.pathToRootEnd(); ++it) {
            stack.emplace(it->functionName);
        }
        if (!tree.processName.empty()) {
            stack.push(tree.processName);
        } else {
            stack.emplace("UNKNOWN");
        }
        while (!stack.empty()) {
            std::string functionName = stack.top();
            stack.pop();
            outputStream << functionName;
            if (!stack.empty()) {
                outputStream << ";";
            }
        }
        outputStream << " " << duration << "\n";
    }
}



#include <thread>

static void parBuild(ParTraceHandle *handle, int threadCount, int threadIndex,
                     CCTree<PerfFoldedNodeData> *out) {
    auto parser = PerfFoldedParser(*handle,
                                   (handle->getFileSize() *  threadIndex     ) / threadCount,
                                   (handle->getFileSize() * (threadIndex + 1)) / threadCount);
    auto builder = Builder<CCTree<PerfFoldedNodeData>>();

    builder.build(out, &parser);
}

CCTree<PerfFoldedNodeData> buildParCCT(const std::string &traceFilePath, int threadCount) {
    auto traceFileHandle = ParTraceHandle(traceFilePath);

    std::vector<CCTree<PerfFoldedNodeData>> trees(threadCount);
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(parBuild, &traceFileHandle, threadCount, i, &trees[i]);
    }

    for (auto &thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads.clear();

    std::cout << "Threads joined. Merging..." << std::endl;
    for (int jump = 1; jump < threadCount; jump *= 2) {
        for (int i = 0; i + jump < threadCount; i += 2 * jump) {
            threads.emplace_back([&trees, i, jump] {
                trees[i].merge(std::move(trees[i + jump]));
            });
        }

        for (auto &thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        threads.clear();
    }

    return std::move(trees[0]);
}



#endif //BUILDER_HPP
