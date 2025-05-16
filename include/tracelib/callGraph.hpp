#ifndef CFG_HPP
#define CFG_HPP

#include <iostream>
#include <sstream>
#include <cstddef>
#include <unordered_set>
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/stack.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>

#include "serialization.hpp"
#include "utils.hpp"

#define AUXILIARY_ROOT_NAME ".ROOT"

// ---------- CCGNode ----------
/**
 * @brief A class representing node of a Connected Call Graph (CCG).
 * @tparam NodeData the type of data stored in the node
 */
template<class NodeData>
class CCGNode {
public:
    using valueType = NodeData;

    /**
     * @brief The name of the function this node corresponds to. It is used as node identifier.
     */
    std::string functionName = "";

    /**
     * @brief Data stored by the user in the node.
     */
    NodeData *data = nullptr;

    /**
     * @brief Creates empty node.
     */
    CCGNode() = default;
    /**
     * @brief Creates new node with specified function name.
     * @param name the function name of the node
     */
    explicit CCGNode(const std::string &name);
    /**
     * @brief Deletes the node with its data as well.
     */
    ~CCGNode();

    /**
     * @brief Forms a string representation of the node.
     * @return string representation of the node
     */
    std::string toString() const;

    /**
     * @brief Compares the nodes based on their function name only. Allows missmatched types of stored data.
     * @tparam U type of stored data in the other node
     * @param other the other node
     * @return true if the function names of both nodes match, false otherwise
     */
    template<typename U>
    bool operator==(CCGNode<U> &other) {
        return this->functionName == other.functionName;
    }

    /**
     * @brief Compares the nodes based on their function name only. Allows missmatched types of stored data.
     * @tparam U type of stored data in the other node
     * @param other the other node
     * @return true if the function names of both nodes do NOT match, false otherwise
     */
    template<typename U>
    bool operator!=(CCGNode<U> &other) {
        return this->functionName != other.functionName;
    }

private:
    friend class boost::serialization::access;

    /**
     * @brief Method that serialized and deserializes this class.
     * @tparam Archive type of the archive
     * @param ar archive to serializa from or deserialize into
     * @param version the version of serialization
     */
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_NVP(functionName);
        ar & BOOST_SERIALIZATION_NVP(data);
    }
};

template<class NodeData>
CCGNode<NodeData>::CCGNode(const std::string &name) : functionName(name) {
    this->data = new NodeData();
}

template<class NodeData>
CCGNode<NodeData>::~CCGNode() {
    delete this->data;
}

template<class NodeData>
std::string CCGNode<NodeData>::toString() const {
    std::stringstream sstream;
    sstream << this->functionName;
    return sstream.str();
}


// ---------- CCGraph ----------
/**
 * @brief A class representing Connected Call Graph (CCG) which is rooted in an auxiliary root to make sure
 * it is connected.
 * @tparam NodeData type of data stored in nodes
 */
template<class NodeData>
class CCGraph {
    using AdjacencyMap = std::unordered_map<std::string, std::unordered_map<std::string, CCGNode<NodeData>*>>;

    /**
     * @brief Auxiliary root that makes sure all nodes are connected to a single graph.
     * For the traces where there is not a single root function from which all functions are called.
     */
    CCGNode<NodeData>* root = nullptr; // auxiliary root
    /**
     * @brief Node that represents currently "executed" function. Function call was already encountered and until
     * there is another function call or function return gathered information is stored within this node.
     */
    CCGNode<NodeData>* currentNode = nullptr;

    /**
     * @brief map of function names of nodes to nodes. The map contains all the nodes in the graph.
     */
    std::unordered_map<std::string, CCGNode<NodeData>*> nodes = {};
    /**
     * @brief mao of function names of nodes to adjacent nodes represented again as a map of functin names to nodes.
     */
    AdjacencyMap adjacencyMap = {};

public:
    using valueType = NodeData;

    /**
     * @brief The name of the process this graph corresponds to.
     */
    std::string processName = "";
    /**
     * @brief The process identifier of the process this graph corresponds to.
     */
    int pid = -1;
    /**
     * @brief The thread identifier of the process this graph corresponds to.
     */
    int tid = -1;

    /**
     * @brief Creates an empty CCG with an auxiliary root.
     */
    CCGraph();

    /**
     * @brief Deletes the whole graph and its data.
     */
    ~CCGraph();

    /**
     * @brief Retrieves the node representing the currently "executing" function.
     * @return current node
     */
    CCGNode<NodeData>* getCurrentNode();

    /**
     * @brief Sets the node representing the currently "executing function".
     * @param node the new current node
     */
    void setCurrentNode(CCGNode<NodeData>* node);

    /**
     * @brief Retrieves the auxiliary root node of the graph.
     * @return auxiliary root node
     */
    CCGNode<NodeData>* getRootNode();

    /**
     * @brief Retrieves the node with specified name.
     * @param name Name of the node to retrieve
     * @return the node with specified name
     */
    CCGNode<NodeData>* getNode(const std::string& name);
    /**
     * @brief Creates a new node with specified name and adds it to the graph structure. It expects that there is
     * not already a node with this name in the graph. Call getNode first.
     * @param name the name of the new node to add to the graph
     * @return the newly created node
     */
    CCGNode<NodeData>* addNewNode(const std::string& name);

    /**
     * @brief Retrieves the child node of current node with specified name.
     * @param name the name of the child node to retrieve
     * @return the child node of current node with specified name
     */
    CCGNode<NodeData>* getChildOfCurrentNode(const std::string& name);
    /**
     * @brief Adds a child to the current noede. Expects that the children of the current node do not contain node
     * with the same name yet. Call getChildOfCurrentNode first.
     * @param node the new child node for the current node
     * @return the new child node (same as passed in)
     */
    CCGNode<NodeData>* addChildToCurrentNode(CCGNode<NodeData>* node);

    /**
     * @brief Retrieves the string representation of the graph.
     * @return string representation of the graph
     */
    std::string toString() const;

    /**
     * @brief Checks if the graph has any additional nodes to the default auxiliary root node.
     * @return true if the graph is has no nodes (except the auxiliary root), false otherwise
     */
    bool isEmpty() const;

    /**
     * @brief Retrieves number of nodes in the graph
     * @return number of nodes in the graph
     */
    long long getNumberOfNodes();

    /**
     * @brief Retrieves number of unique function in the graph
     * @return number of unique functions in the graph
     */
    long long getNumberOfFunctions();

    /**
     * @brief Checks if both graphs have the same nodes. Checks only the function names within the nodes, meaning the
     * structure of the graph is checked rather than the contents of the nodes.
     * @tparam U type of the data stored in nodes of the other graph
     * @param other other graph
     * @return true if structure of the graphs is the same, false otherwise
     */
    template<typename U>
    bool operator==(CCGraph<U> &other) {
        auto it1 = this->begin();
        auto it2 = other.begin();
        bool areEqual = true;
        while (it1 != this->end()) {
            if (it2 == other.end()) {
                areEqual = false;
                break;
            }
            auto* node1 = *it1;
            auto* node2 = *it2;
            if (*node1 != *node2) {
                areEqual = false;
                break;
            }
            ++it1;
            ++it2;
        }
        return areEqual;
    }

    /**
     * @brief Checks if both graphs have the same nodes. Checks only the function names within the nodes, meaning the
     * structure of the graph is checked rather than the contents of the nodes.
     * @tparam U type of the data stored in nodes of the other graph
     * @param other other graph
     * @return true if structure of the graphs is NOT the same, false otherwise
     */
    template<typename U>
    bool operator!=(CCGraph<U> &other) {
        return !(*this == other);
    }

    /**
     * @brief Breath First Search iterator over the graph nodes.
     */
    class BFSIterator {
    private:
        /**
         * @brief The node currently pointed to by the iterator.
         */
        CCGNode<NodeData>* currentNode = nullptr;
        /**
         * @brief Queue for nodes that will be visited next.
         */
        std::deque<CCGNode<NodeData>*> queue{};
        /**
         * @brief Set of visited node names.
         */
        std::unordered_set<std::string> visited{};
        /**
         * @brief Adjacency map of of the graph.
         */
        AdjacencyMap* adjacencyMap = nullptr;
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = CCGNode<NodeData>*;
        using pointer = CCGNode<NodeData>*;
        using reference = CCGNode<NodeData>&;

        explicit BFSIterator(AdjacencyMap* adjacencyMap, CCGNode<NodeData>* node) :
        currentNode(node), adjacencyMap(adjacencyMap) {
            if (this->currentNode == nullptr) {
                return;
            }
            this->currentNode = node;
            this->queue.push_back(this->currentNode);
            this->visited.insert(this->currentNode->functionName);
        }
        BFSIterator() = default;

        CCGNode<NodeData>* operator*() const { return currentNode; }
        CCGNode<NodeData>* operator->() { return currentNode; }

        BFSIterator& operator++() { // Prefix increment
            if (this->queue.empty()) {
                this->currentNode = nullptr;
                return *this;
            }
            auto* node = this->queue.front();
            this->queue.pop_front();

            // Add children of the visited node to the queue
            auto it = adjacencyMap->find(node->functionName);
            if (it != adjacencyMap->end()) {
                // Sort the keys for deterministic iteration
                std::vector<std::string> adjacentNodeNames;
                for (auto& [name, adjacentNode] : it->second) {
                    adjacentNodeNames.push_back(name);
                }
                std::ranges::sort(adjacentNodeNames);
                for (std::string& adjacentNodeName : adjacentNodeNames) {
                    if (this->visited.find(adjacentNodeName) == this->visited.end()) {
                        this->queue.push_back(it->second[adjacentNodeName]);
                        this->visited.insert(adjacentNodeName);
                    }
                }
            }
            // Update the current node
            this->currentNode = this->queue.empty() ? nullptr : this->queue.front();
            return *this;
        }
        BFSIterator operator++(int) { // Postfix increment
            BFSIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const BFSIterator& a, const BFSIterator& b) { return a.currentNode == b.currentNode; }
        friend bool operator!=(const BFSIterator& a, const BFSIterator& b) { return a.currentNode != b.currentNode; }
    };

    /**
     * @brief Depth First Search iterator over the graph nodes.
     */
    class DFSIterator {
    private:
        /**
         * @brief The node currently pointed to by the iterator.
         */
        CCGNode<NodeData>* currentNode = nullptr;
        /**
         * @brief Stack of nodes to be visited by the iterator
         */
        std::stack<CCGNode<NodeData>*> stack{};
        /**
         * @brief Set of nodes that were already visited by the iterator.
         */
        std::unordered_set<std::string> visited{};
        /**
         * @brief The adjacency map of the graph nodes.
         */
        AdjacencyMap* adjacencyMap = nullptr;
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = CCGNode<NodeData>;
        using pointer = CCGNode<NodeData>*;
        using reference = CCGNode<NodeData>&;

        explicit DFSIterator(AdjacencyMap* adjacencyMap, CCGNode<NodeData>* node) :
        currentNode(node), adjacencyMap(adjacencyMap) {
            if (this->currentNode == nullptr) {
                return;
            }
            this->currentNode = node;
            this->stack.push(this->currentNode);
            this->visited.insert(this->currentNode->functionName);
        }
        DFSIterator() = default;

        CCGNode<NodeData>* operator*() const { return currentNode; }
        CCGNode<NodeData>* operator->() { return currentNode; }

        DFSIterator& operator++() { // Prefix increment
            if (this->stack.empty()) {
                this->currentNode = nullptr;
                return *this;
            }
            auto* node = this->stack.top();
            this->stack.pop();

            // Add children of the visited node to the queue
            auto it = adjacencyMap->find(node->functionName);
            if (it != adjacencyMap->end()) {
                // Sort the keys for deterministic iteration
                std::vector<std::string> adjacentNodeNames;
                for (auto& [name, adjacentNode] : it->second) {
                    adjacentNodeNames.push_back(name);
                }
                std::sort(adjacentNodeNames.begin(), adjacentNodeNames.end(), std::greater<>());
                for (std::string& adjacentNodeName : adjacentNodeNames) {
                    if (this->visited.find(adjacentNodeName) == this->visited.end()) {
                        this->stack.push(it->second[adjacentNodeName]);
                        this->visited.insert(adjacentNodeName);
                    }
                }
            }
            // Update the current node
            this->currentNode = this->stack.empty() ? nullptr : this->stack.top();
            return *this;
        }
        DFSIterator operator++(int) { // Postfix increment
            DFSIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const DFSIterator& a, const DFSIterator& b) { return a.currentNode == b.currentNode; }
        friend bool operator!=(const DFSIterator& a, const DFSIterator& b) { return a.currentNode != b.currentNode; }
    };

    using defaultIterator = BFSIterator;
    /**
     * @brief Breath First Search iterator over nodes in the graph starting in the auxiliary root node.
     * @return BFS iterator from the begining of the graph (auxiliary root)
     */
    defaultIterator begin() { return defaultIterator(&adjacencyMap, this->root); }
    /**
     * @brief Breath First Search iterator over nodes in the graph starting in the specified node.
     * @param root the node in which to start the BFS iterator
     * @return BFS iterator from the specified node
     */
    defaultIterator begin(CCGNode<NodeData>* root) { return defaultIterator(&adjacencyMap, root); }
    /**
     * @brief BFS iterator end.
     * @return BFS iterator representing the end of iteration
     */
    defaultIterator end() { return defaultIterator(); }

    /**
     * @brief Breath First Search iterator over nodes in the graph starting in the auxiliary root node.
     * @return BFS iterator from the begining of the graph (auxiliary root)
     */
    BFSIterator bfsBegin() { return BFSIterator(&adjacencyMap, this->root); }
    /**
     * @brief Breath First Search iterator over nodes in the graph starting in the specified node.
     * @param root the node in which to start the BFS iterator
     * @return BFS iterator from the specified node
     */
    BFSIterator bfsBegin(CCGNode<NodeData>* root) { return BFSIterator(&adjacencyMap, root); }
    /**
     * @brief BFS iterator end.
     * @return BFS iterator representing the end of iteration
     */
    BFSIterator bfsEnd() { return BFSIterator(); }

    /**
     * @brief Depth First Search iterator over nodes in the graph starting in the auxiliary root node.
     * @return DFS iterator from the begining of the graph (auxiliary root)
     */
    DFSIterator dfsBegin() { return DFSIterator(&adjacencyMap, this->root); }
    /**
     * @brief Depth First Search iterator over nodes in the graph starting in the specified node.
     * @param root the node in which to start the DFS iterator
     * @return DFS iterator from the specified node
     */
    DFSIterator dfsBegin(CCGNode<NodeData>* root) { return DFSIterator(&adjacencyMap, root); }
    /**
     * @brief DFS iterator end.
     * @return DFS iterator representing the end of iteration
     */
    DFSIterator dfsEnd() { return DFSIterator(); }

private:
    friend class boost::serialization::access;

    /**
     * @brief Serialization function for this class
     * @tparam Archive type of the archive
     * @param ar archive to serialize into
     * @param version version of the serialization
     */
    template<class Archive>
    void save(Archive &ar, const unsigned int version) const {
        ar & BOOST_SERIALIZATION_NVP(root);
        ar & BOOST_SERIALIZATION_NVP(currentNode);
        ar & BOOST_SERIALIZATION_NVP(adjacencyMap);
        ar & BOOST_SERIALIZATION_NVP(nodes);
        ar & BOOST_SERIALIZATION_NVP(processName);
        ar & BOOST_SERIALIZATION_NVP(pid);
        ar & BOOST_SERIALIZATION_NVP(tid);
    }

    /**
     * @brief Deserialization function for this class
     * @tparam Archive type of the archive
     * @param ar archive to deserialize from
     * @param version version of the serialization
     */
    template<class Archive>
    void load(Archive &ar, const unsigned int version) {
        delete this->root; //delete the auxiliary root node
        this->root = nullptr;
        this->currentNode = nullptr;
        this->nodes.clear();
        this->adjacencyMap.clear();

        ar & BOOST_SERIALIZATION_NVP(root);
        ar & BOOST_SERIALIZATION_NVP(currentNode);
        ar & BOOST_SERIALIZATION_NVP(adjacencyMap);
        ar & BOOST_SERIALIZATION_NVP(nodes);
        ar & BOOST_SERIALIZATION_NVP(processName);
        ar & BOOST_SERIALIZATION_NVP(pid);
        ar & BOOST_SERIALIZATION_NVP(tid);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER() // Allows to separate defualt serialize function into save and load functions.
};

template<class NodeData>
CCGraph<NodeData>::CCGraph() {
    this->root = new CCGNode<NodeData>(AUXILIARY_ROOT_NAME);
    this->nodes[AUXILIARY_ROOT_NAME] = this->root;
    adjacencyMap[AUXILIARY_ROOT_NAME] = std::unordered_map<std::string, CCGNode<NodeData>*>();
    this->currentNode = this->root;
}

template<class NodeData>
CCGraph<NodeData>::~CCGraph() {
    for (auto& [nodeName, node] : this->nodes) {
        delete node;
    }
    this->nodes.clear();
    this->adjacencyMap.clear();
}

template<class NodeData>
CCGNode<NodeData>* CCGraph<NodeData>::getCurrentNode() {
    return this->currentNode;
}

template<class NodeData>
void CCGraph<NodeData>::setCurrentNode(CCGNode<NodeData> *node) {
    this->currentNode = node;
}

template<class NodeData>
CCGNode<NodeData> * CCGraph<NodeData>::getRootNode() {
    return this->root;
}

template<class NodeData>
CCGNode<NodeData> * CCGraph<NodeData>::getNode(const std::string &name) {
    auto it = this->nodes.find(name);
    if (it != this->nodes.end()) {
        return it->second;
    }
    return nullptr;
}

template<class NodeData>
CCGNode<NodeData>* CCGraph<NodeData>::addNewNode(const std::string &name) {
    // Note: expects that there is not a node with the same name (call getNode first)
    auto *newNode = new CCGNode<NodeData>(name);
    this->nodes.emplace(name, newNode);
    //this->adjacencyList.emplace(name, std::unordered_map<std::string, CCGNode<NodeData>*>{});
    return newNode;
}

template<class NodeData>
CCGNode<NodeData> * CCGraph<NodeData>::getChildOfCurrentNode(const std::string &name) {
    auto it = this->adjacencyMap.find(this->currentNode->functionName);
    if (it != this->adjacencyMap.end()) {
        auto& adjacentNodes = it->second;
        auto childIt = adjacentNodes.find(name);
        if (childIt != adjacentNodes.end()) {
            return childIt->second;
        }
    }
    return nullptr;
}

template<class NodeData>
CCGNode<NodeData>* CCGraph<NodeData>::addChildToCurrentNode(CCGNode<NodeData>* node) {
    auto it = this->adjacencyMap.find(this->currentNode->functionName);
    if (it == this->adjacencyMap.end()) {
        auto out = this->adjacencyMap.emplace(this->currentNode->functionName, std::unordered_map<std::string, CCGNode<NodeData>*>{});
        it = out.first;
    }
    // Note: expects that the node name does not exist in adjacent nodes yet
    auto& adjacentNodesMap = it->second;
    adjacentNodesMap.emplace(node->functionName, node);
    return node;
}

template<class NodeData>
std::string CCGraph<NodeData>::toString() const {
    std::stringstream sstream;
    sstream << "PID: " << this->pid << " TID: " << this->tid << " Process name:" << this->processName << std::endl;
    sstream << "Nodes: ";
    bool first = true;
    for (auto& [name, node] : this->nodes) {
        if (!first) {
            sstream << ", ";
        }
        sstream << name;
        first = false;
    }
    sstream << "\n";

    sstream << "Adjacency list: \n";
    for (auto& [name, adjacentNodes] : this->adjacencyMap) {
        sstream << name << ": ";
        bool first = true;
        for (auto& [adjacentNodeName, adjacentNode] : adjacentNodes) {
            if (!first) {
                sstream << ", ";
            }
            sstream << adjacentNodeName;
            first = false;
        }
        sstream << "\n";
    }
    return sstream.str();
}

template<class NodeData>
bool CCGraph<NodeData>::isEmpty() const {
    return this->nodes.size() <= 1 and this->adjacencyMap.size() <= 1;
}

template<class NodeData>
long long CCGraph<NodeData>::getNumberOfNodes() {
    return this->nodes.size();
}

template<class NodeData>
long long CCGraph<NodeData>::getNumberOfFunctions() {
    return this->getNumberOfNodes();
}

// ---------- DCGraph ----------
/**
 * @brief A class representing Disconnected Call Graph (DCG) as a set of Connected Call Graphs (CCGs).
 * @tparam NodeData the type of the data stored in the nodes of the CCGs
 */
template<class NodeData>
class DCGraph {
private:
    /**
     * @brief Map of graphs based on the process identifier (pid) and thread identifier (tid). Basically represents
     * a Disconnected Call Graph with set of Connected Call Graphs.
     */
    std::unordered_map<std::pair<int, int>, CCGraph<NodeData>*, PairHash> graphs = {};
public:
    using valueType = NodeData;

    /**
     * @brief Creates empty disconnected call graph.
     */
    DCGraph() = default;

    /**
     * @brief Deletes the whole graph and its data.
     */
    ~DCGraph();

    /**
     * @brief Retrieves a sub-graph (CCG) that is identified by the specified pid and tid pair
     * @param pid process identifier of the sub-graph
     * @param tid thread identifier of the sub-graph
     * @return the sub-graph identified by selected pid and tid pair
     */
    CCGraph<NodeData>* getGraph(int pid, int tid);

    /**
     * @brief Adds a new sub-graph to the graph based on its pid and tid. Expects pid and tid specified in the graph.
     * @param graph new sub-graph to add to the graph
     */
    void addGraph(CCGraph<NodeData>* graph);

    /**
     * @brief Creates a new graph with specified pid, tid and process name and adds it to the set of graphs that
     * together create the DCG. Expects the pid and tid to not be in the DCG yet. Call getGraph first.
     * @param pid process identifier of the sub-graph
     * @param tid thread identifier of the sub-graph
     * @param processName process name of the sub-graph
     * @return newly created sub-graph
     */
    CCGraph<NodeData>* addNewGraph(int pid, int tid, const std::string& processName = "");

    /**
     * @brief Forms string representation of the graph.
     * @return string representation of the graph
     */
    std::string toString() const;

    /**
     * @brief Checks if the DCGraph contains any CCG sub-graphs.
     * @return true if the graph does not contain any CCG sub-graphs, flase otherwise
     */
    bool isEmpty() const;

    /**
     * @brief Retrieve number of nodes from all of the sub-structures
     * @return number of nodes in the whole structure
     */
    long long getNumberOfNodes();

    /**
     * @brief Retrieve number of unique functions from all of the sub-structures
     * @return number of unique function in the whole structure
     */
    long long getNumberOfFunctions();

    /**
     * @brief Checks if both graphs have the same structure, does not check contents of the nodes only their identifiers.
     * @tparam U type of data stored in nodes of the other graph
     * @param other the other graph
     * @return true if both graphs have the same structure, flase otherwise
     */
    template<typename U>
    bool operator==(DCGraph<U> &other) {
        auto it1 = this->begin();
        auto it2 = other.begin();
        bool areEqual = true;
        while (it1 != this->end()) {
            if (it2 == other.end()) {
                areEqual = false;
                break;
            }
            auto* graph1 = it1->second;
            auto* graph2 = it2->second;
            if (*graph1 != *graph2) {
                areEqual = false;
                break;
            }
            ++it1;
            ++it2;
        }
        return areEqual;
    }

    /**
     * @brief Checks if both graphs have the same structure, does not check contents of the nodes only their identifiers.
     * @tparam U type of data stored in nodes of the other graph
     * @param other the other graph
     * @return true if graphs have different structure, flase otherwise
     */
    template<typename U>
    bool operator!=(DCGraph<U> &other) {
        return !(*this == other);
    }

    /**
     * @brief Iterator over all the Connected Call Graphs in this Disconnected Call Graph.
     */
    class GraphIterator {
    private:
        using GraphsMap = std::unordered_map<std::pair<int, int>, CCGraph<NodeData>*, PairHash>;

        /**
         * @brief Ordered keys from the GraphsMap for deterministic iteration.
         */
        std::vector<std::pair<int, int>> keys = {};
        /**
         * @brief Map of CCGs within the DCG.
         */
        GraphsMap* graphs;
        /**
         * @brief The current key index within the keys vector.
         */
        size_t currentKeyIndex = 0;
        /**
         * @brief The pair of key and value from the graphs map the iterator is pointing on currently.
         */
        std::pair<std::pair<int, int>, CCGraph<NodeData>*> currentPair = {};

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<std::pair<int, int>, CCGraph<NodeData>*>;
        using pointer = std::pair<std::pair<int, int>, CCGraph<NodeData>*>*;
        using reference = std::pair<std::pair<int, int>, CCGraph<NodeData>*>&;

        explicit GraphIterator(GraphsMap* graphs, bool isEnd = false) : graphs(graphs) {
            size_t graphsSize = graphs->size();
            if (isEnd) {
                this->currentKeyIndex = graphs != nullptr ? graphsSize : 0;
                return;
            }

            // Sort the keys for deterministic iteration
            this->keys.reserve(graphsSize);
            for (const auto& [key, _] : *graphs) {
                this->keys.push_back(key);
            }
            std::sort(this->keys.begin(), this->keys.end());

            this->currentKeyIndex = 0;
            auto& currentKey = this->keys[this->currentKeyIndex];
            this->currentPair = std::make_pair(currentKey, this->graphs->at(currentKey));
        }

        reference operator*() { return this->currentPair; }
        pointer operator->() { return &this->currentPair; }

        GraphIterator& operator++() { // Prefix increment
            ++this->currentKeyIndex;
            if (this->currentKeyIndex >= this->keys.size()) {
                this->currentKeyIndex = this->keys.size();
                this->currentPair = {};
                return *this;
            }
            auto& currentKey = this->keys[this->currentKeyIndex];
            this->currentPair = std::make_pair(currentKey, this->graphs->at(currentKey));
            return *this;
        }
        GraphIterator operator++(int) { // Postfix increment
            GraphIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const GraphIterator& a, const GraphIterator& b) {
            return a.currentKeyIndex == b.currentKeyIndex;
        }
        friend bool operator!=(const GraphIterator& a, const GraphIterator& b) {
            return a.currentKeyIndex != b.currentKeyIndex;
        }
    };

    using defaultIterator = GraphIterator;
    /**
     * @brief Start of iterator over Connected Call Graphs (sub-graphs) in this Disconnected Call Graph.
     * @return Iterator starting at the graph with smalles pid and tid pair
     */
    defaultIterator begin() { return defaultIterator(&this->graphs); }
    /**
     * @brief End of sub-graph itarator
     * @return iterator representing the end of iteration over the sub-graphs
     */
    defaultIterator end() { return defaultIterator(&this->graphs, true); }

private:
    friend class boost::serialization::access;

    /**
     * @brief Serialization and deserialization function for this class.
     * @tparam Archive type of the archive
     * @param ar the archive to serialize into and deserialize from
     * @param version serialization version
     */
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_NVP(this->graphs);
    }
};

template<class NodeData>
DCGraph<NodeData>::~DCGraph() {
    for (auto&[pid_tid, tree] : this->graphs) {
        delete tree;
        tree = nullptr;
    }
}

template<class NodeData>
CCGraph<NodeData> * DCGraph<NodeData>::getGraph(int pid, int tid) {
    auto key = std::make_pair(pid, tid);
    if (auto it = this->graphs.find(key); it != this->graphs.end()) {
        return it->second;
    }
    return nullptr;
}

template<class NodeData>
void DCGraph<NodeData>::addGraph(CCGraph<NodeData> *graph) {
    auto key = std::make_pair(graph->pid, graph->tid);
    if (const auto it = this->graphs.find(key); it != this->graphs.end()) {
        std::cerr << "[W]: A graph with specified pid and tid already exists! "
                  << "Will not add a graph with the same process identification to the multi graph." << std::endl;
        return;
    }
    this->graphs.emplace(key, graph);
}

template<class NodeData>
CCGraph<NodeData> * DCGraph<NodeData>::addNewGraph(int pid, int tid, const std::string &processName) {
    // Note: expects the graph to be new (pid and tid not in the graphs yet)
    auto* graph = new CCGraph<NodeData>();
    graph->processName = processName;
    graph->pid = pid;
    graph->tid = tid;
    this->graphs.emplace(std::make_pair(pid, tid), graph);
    return graph;
}

template<class NodeData>
std::string DCGraph<NodeData>::toString() const {
    std::stringstream sstream;
    for (auto& [id, graph] : graphs) {
        sstream << graph->toString() << "\n";
    }
    return sstream.str();
}

template<class NodeData>
bool DCGraph<NodeData>::isEmpty() const {
    return graphs.empty();
}

template<class NodeData>
long long DCGraph<NodeData>::getNumberOfNodes() {
    long long cnt = 0;
    for (auto& [id, graph] : graphs) {
        cnt += graph->getNumberOfNodes();
    }
    return cnt;
}

template<class NodeData>
long long DCGraph<NodeData>::getNumberOfFunctions() {
    std::unordered_set<std::string> uniqueFunctionNames = {};
    for (auto&[id, graph] : graphs) {
        for (auto it = graph->begin(); it != graph->end(); ++it) {
            CCGNode<NodeData>* node = *it;
            uniqueFunctionNames.insert(node->functionName);
        }
    }
    return uniqueFunctionNames.size();
}


#endif //CFG_HPP
