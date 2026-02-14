#ifndef CCT_HPP
#define CCT_HPP

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>
#include <memory>
#include <iterator>
#include <cstddef>
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/unique_ptr.hpp>

#include "utils.hpp"

#define AUXILIARY_ROOT_ID 0
#define AUXILIARY_ROOT_NAME ".ROOT"

using functionIdType = uint32_t;

template <class NodeData>
class CCTree;

/**
 * @brief A class representing a node in Calling Context Tree (CCT) structure.
 * @tparam NodeData A derived class from NodeData. Although this is not enforced, the Builder class expects the methods
 * defined by the NodeData class to be implemented on the data field.
 */
template<class NodeData>
class CCTNode {
public:
    using valueType = NodeData;

    /**
     * @brief The node is identified by the function id mapping to a name in the CCTree.
     * The name ".ROOT" is reserved for the auxiliary root of the CCT structure.
     */
    functionIdType functionId;

    /**
     * @brief The node data should be an instance of class derived from NodeData class.
     */
    std::unique_ptr<NodeData> data;

    /**
     * @brief The parent node of this node.
     */
    CCTNode* parent = nullptr;

    /**
     * @brief The children of this node in a map where the key is nodes function name.
     */
    std::unordered_map<functionIdType, std::unique_ptr<CCTNode>> children = {};

    /**
     * @brief Creates an empty node.
     */
    CCTNode() = default;

    /**
     * @brief Creates a node with specified function name and optionally a parent pointer. The node
     * will have no children.
     * @param name the function name for the new node
     * @param parent parent node of the node
     */
    explicit CCTNode(functionIdType id, CCTNode* parent = nullptr);

    /**
     * @brief Destructor. Deallocates the node data and all its children.
     */
    ~CCTNode();

    /**
     * @brief Adds the specified child node to the children of this node.
     * @param child a new child node
     */
    CCTNode<NodeData> *addChild(std::unique_ptr<CCTNode<NodeData>> &&child);

    /**
     * @brief Removes the specified child node by its function id from the children of this node. The node
     * is deallocated as well.
     * @param childId a child node function id
     */
    void removeChild(functionIdType childId);

    /**
     * @brief Returns the child node with the specified function id.
     * @param childId a child node function id
     * @return the child node with the specified function id
     */
    CCTNode<NodeData>* getChild(functionIdType childId);

    /**
     * @brief Returns the child node with the specified function id, insert new if not found.
     * @param childId a child node function id
     * @return the child node with the specified function id
     */
    CCTNode<NodeData>* getChildInsert(functionIdType childId);

    /**
     * @brief Merge other CCTNode into the current one.
     */
    void merge(std::unique_ptr<CCTNode<NodeData>>&& other);

    /**
     * @brief Recursively remap node's (and its children's) function id from the current map, into new tree's map.
     */
    void remapFunctionId(const std::vector<std::string> &oldMap, CCTree<NodeData> &newTree);

    /**
     * @brief Comparison of nodes that is checking only the function id and is allows missmatched NodeData types.
     * @tparam U the type of the other node
     * @param other the other node
     * @return true if the nodes ahve the same function id
     */
    template <typename U>
    bool operator==(CCTNode<U>& other) {
        return this->functionId == other.functionId;
    }

    /**
     * @brief Comparison of nodes that is checking only the function id and is allows missmatched NodeData types.
     * @tparam U the type of the other node
     * @param other the other node
     * @return true if the nodes have different function ids
     */
    template <typename U>
    bool operator!=(CCTNode<U>& other) {
        return this->functionId != other.functionId;
    }

private:
    friend class boost::serialization::access;

    /**
     * @brief Save method used for serialization of the node instance. Should not be called directly.
     * @tparam Archive the type of the archive to serialize into
     * @param ar the archive to serialize into
     * @param version the version of the serialization format
     */
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const {
        ar & BOOST_SERIALIZATION_NVP(functionId);
        ar & BOOST_SERIALIZATION_NVP(data);
        ar & BOOST_SERIALIZATION_NVP(children);
        // Doesn't serialize parent (handled by child relationship in the parent)
    }

    /**
     * @brief Load method used for serialization of the node instance. Should not be called directly.
     * @tparam Archive the type of the archive to serialize into
     * @param ar the archive to serialize into
     * @param version the version of the serialization format
     */
    template<class Archive>
    void load(Archive & ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_NVP(functionId);
        ar & BOOST_SERIALIZATION_NVP(data);
        ar & BOOST_SERIALIZATION_NVP(children);
        // Fix parent pointers after loading children
        for (auto& pair : children) {
            pair.second->parent = this;
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER() // Allows to split default serialization function into save and load functions
};

template<class NodeData>
CCTNode<NodeData>::CCTNode(functionIdType id, CCTNode *parent) : functionId(id), parent(parent),
                                                         data(std::make_unique<NodeData>()) {
}

template<class NodeData>
CCTNode<NodeData>::~CCTNode() {
}

template<class NodeData>
CCTNode<NodeData> *CCTNode<NodeData>::addChild(std::unique_ptr<CCTNode<NodeData>> &&child) {
    auto ret = this->children.emplace(child->functionId, std::forward<std::unique_ptr<CCTNode<NodeData>>>(child)).first->second.get();
    ret->parent = this;
    return ret;
}

template<class NodeData>
void CCTNode<NodeData>::removeChild(functionIdType childId) {
    this->children.erase(childId);
}

template<class NodeData>
CCTNode<NodeData> * CCTNode<NodeData>::getChild(functionIdType childId) {
    auto it = this->children.find(childId);
    return it != this->children.end() ? it->second.get() : nullptr;
}

template<class NodeData>
CCTNode<NodeData> *CCTNode<NodeData>::getChildInsert(functionIdType childId) {
    if (auto child = this->children.find(childId); child != this->children.end()) {
        return child->second.get();
    }
    return this->children.emplace(childId, std::make_unique<CCTNode<NodeData>>(childId, this)).first->second.get();
}

template<class NodeData>
void CCTNode<NodeData>::merge(std::unique_ptr<CCTNode<NodeData>> &&other)
{
    if (other->data != nullptr) {
        if (this->data == nullptr) {
            this->data = std::forward<std::unique_ptr<NodeData>>(other->data);
        } else {
            this->data->merge(std::forward<std::unique_ptr<NodeData>>(other->data));
        }
        other->data = nullptr;
    }

    for (auto &[_, node] : other->children) {
        if (node == nullptr) {
            continue;
        }

        if (auto child = this->children.find(node->functionId); child != this->children.end()) {
            child->second->merge(std::forward<std::unique_ptr<CCTNode<NodeData>>>(node));
        } else {
            addChild(std::forward<std::unique_ptr<CCTNode<NodeData>>>(node));
        }
        node = nullptr;
    }
    other = nullptr;
}


/**
 * @brief A class representing a Calling Context Tree (CCT).
 * @tparam NodeData A derived class from NodeData. Although this is not enforced, the Builder class expects the methods
 * defined by the NodeData class to be implemented on the data field.
 */
template <class NodeData>
class CCTree {
private:
    /**
     * @brief The root node of the tree.
     * By default it is always an auxiliary root node with function name ".ROOT".
     */
    std::unique_ptr<CCTNode<NodeData>> root;
    /**
     * @brief The node which represents the currently "executed" function. The function call was encountered
     * and every event happening until a new function call or function returns is associated with this node.
     * It is used to build the tree based on the events.
     */
    CCTNode<NodeData>* currentNode;

    struct TransparentStringHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const noexcept {
            return std::hash<std::string_view>{}(sv);
        }
    };
    std::unordered_map<std::string, functionIdType, TransparentStringHash, std::equal_to<>> functionNameToIdMap;
    std::vector<std::string> functionIdToNameMap;

    std::pair<functionIdType, bool> functionNameToId(const std::string &name) const {
        auto it = functionNameToIdMap.find(name);
        if (it == functionNameToIdMap.end()) {
            return {0, false};
        }
        auto [_, val] = *it;
        return {val, true};
    }
    std::pair<functionIdType, bool> functionNameToId(std::string_view name) const {
        auto it = functionNameToIdMap.find(name);
        if (it == functionNameToIdMap.end()) {
            return {0, false};
        }
        auto [_, val] = *it;
        return {val, true};
    }

public:
    functionIdType functionNameToIdInsert(const std::string &name) {
        auto [it, wasNew] = functionNameToIdMap.try_emplace(name, functionIdToNameMap.size());
        if (wasNew) {
            functionIdToNameMap.emplace_back(name);
        }
        auto [_, val] = *it;
        return val;
    }
    functionIdType functionNameToIdInsert(std::string_view name) {
        if (auto search = functionNameToIdMap.find(name); search != functionNameToIdMap.end()) {
            return search->second;
        }
        auto [it, _] = functionNameToIdMap.emplace(std::string(name), functionIdToNameMap.size());
        functionIdToNameMap.emplace_back(name);
        return it->second;
    }

    const std::string &functionIdToName(functionIdType id) const {
        return functionIdToNameMap.at(id);
    }

    using valueType = NodeData;

    /**
     * @brief Name of the process that this tree belongs to.
     */
    std::string processName = "";
    /**
     * @brief The Process identifier of the process that this tree belongs to.
     */
    int pid = -1;
    /**
     * @brief The Thread identifier of the thread that this tree belongs to.
     */
    int tid = -1;

    /**
     * @brief Creates a new CCT with an auxiliary root node. This is enforced to make sure that
     * traces that have not single root function (e.g. main function) form a valid tree.
     */
    CCTree();
    /**
     * @brief Deletes the tree entirely.
     */
    ~CCTree();

    CCTree(CCTree &&) = default;

    /**
     * @brief Retrieves the current node.
     * @return current node
     */
    CCTNode<NodeData>* getCurrentNode();
    /**
     * @brief Sets the current node.
     * @param node the new current node
     */
    void setCurrentNode(CCTNode<NodeData>* node);

    /**
     * @brief Retrieves the root node of the tree.
     * @return root node
     */
    const CCTNode<NodeData>* getRootNode() const;
    CCTNode<NodeData>* getRootNode();

    /**
     * @brief Retrieves the parent of the current node.
     * @return parent node of current node
     */
    CCTNode<NodeData>* getParentOfCurrentNode();

    /**
     * @brief Retrieves the child of the current node with the specified id.
     * @param childId the id of the child
     * @return child node with specified id
     */
    CCTNode<NodeData>* getChildOfCurrentNode(const std::string &name);
    CCTNode<NodeData>* getChildOfCurrentNode(std::string_view name);

    /**
     * @brief Creates a new node with specified id and adds it to the children of current node.
     * @param childId the id of the new node
     * @return the nely created node
     */
    CCTNode<NodeData>* addNewChildToCurrentNode(const std::string &name);
    //void removeNode(CCTNode<NodeData>* node);

    /**
     * @brief Prunes the tree starting from leaves based on the specified threshold.
     * @param threshold the threshold value of occurrence frequency for functions in their contexts
     */
    void prune(long long int threshold);
    std::pair<int, std::vector<Operation<CCTNode<NodeData>>>> treeEditDistance(CCTree<NodeData>& other);

    /**
     * @brief Merge other CCTree into the current one.
     */
    void merge(CCTree<NodeData> &&other);

    /**
     * @brief Form string representation of the tree.
     * @return string representation of the tree
     */
    std::string toString() const;

    /**
     * @brief Checks if the tree does not have any nodes except the auxiliary root node.
     * @return true if the tree is empty, false otherwise
     */
    bool isEmpty() const;

    /**
     * @brief Retrieves number of nodes in the tree.
     * @return number of nodes in the tree
     */
    long long getNumberOfNodes();

    /**
     * @brief Retrieves number of unique functions in the tree.
     * @return number of unique functions in the tree
     */
    long long getNumberOfFunctions();

    /**
     * @brief Retrieves maximal number of invocations of a node in the tree -- the maximal number
     * of calls a function in the tree has.
     * @return number of maximal invocations of a function from the tree
     */
    long long getMaximumInvocations();

    /**
     * @brief Comparison of trees that is checking the structure of the tree only. It uses the comparison of nodes
     * and thus only compares their function ids and not the data they hold. Hence, it is allowed to missmatched
     * NodeData types.
     * @tparam U the type of the data in nodes of the other tree
     * @param other the other tree
     * @return true if the trees are equal in structure, false otherwise
     */
    template <typename U>
    bool operator==(CCTree<U>& other) {
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
     * @brief Comparison of trees that is checking the structure of the tree only. It uses the comparison of nodes
     * and thus only compares their function ids and not the data they hold. Hence, it is allowed to missmatched
     * NodeData types.
     * @tparam U the type of the data in nodes of the other tree
     * @param other the other tree
     * @return true if the trees are NOT equal in structure, false otherwise
     */
    template <typename U>
    bool operator!=(CCTree<U>& other) {
        return !(*this == other);
    }


    // Iterators
    /**
     * @brief Iterator for traversing the tree in pre-order.
     */
    class PreOrderIterator {
    private:
        /**
         * @brief The node that is pointed to by the iterator.
         */
        CCTNode<NodeData>* currentNode;
        /**
         * @brief Stack used for storing the nodes that are to be visited.
         */
        std::stack<CCTNode<NodeData>*> stack;
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = CCTNode<NodeData>*;
        using pointer = CCTNode<NodeData>*;
        using reference = CCTNode<NodeData>&;

        explicit PreOrderIterator(CCTNode<NodeData>* node = nullptr) : currentNode(node) {
            if (node != nullptr) {
                stack.push(node);
            }
        }

        CCTNode<NodeData>* operator*() const { return currentNode; }
        CCTNode<NodeData>* operator->() { return currentNode; }

        PreOrderIterator& operator++() { // Prefix increment
            // Push children of current node on stack
            CCTNode<NodeData>* node = stack.top();
            stack.pop();
            for (auto &[_, child] : node->children) {
                stack.emplace(child.get());
            }
            // Update current node
            this->currentNode = stack.empty() ? nullptr : stack.top();
            return *this;
        }
        PreOrderIterator operator++(int) { // Postfix increment
            PreOrderIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const PreOrderIterator &a, const PreOrderIterator &b) { return a.currentNode == b.currentNode; }
        friend bool operator!=(const PreOrderIterator &a, const PreOrderIterator &b) { return a.currentNode != b.currentNode; }
    };

    /**
     * @brief Iterator for traversing the tree in post-order.
     */
    class PostOrderIterator {
    private:
        /**
         * @brief The current node that is pointed to by the iterator.
         */
        CCTNode<NodeData>* currentNode;
        /**
         * @brief Stack that is used for storing the nodes that are to be visited.
         * The bool value indicates whether the node has been visited or not in context of the internal traversal
         * algorithm. When a node with the bool value true is popped from the stack it is set to the current node.
         */
        std::stack<std::pair<CCTNode<NodeData>*, bool>> stack;

        /**
         * @brief helper funtion to find the leftmost child of the current node with relation to already visited nodes.
         */
        void exploreUntilLeftmostChild() {
            while (!this->stack.empty()) {
                auto& [node, visited] = this->stack.top();
                if (!visited) {
                    visited = true;
                    for (auto it = node->children.rbegin(); it != node->children.rend(); ++it) {
                        auto* child = it->second.get();
                        if (child != nullptr) {
                            this->stack.push({child, false});
                        }
                    }
                } else { // The leftmost child
                    this->currentNode = node;
                    return;
                }
            }
            this->currentNode = nullptr;
        }
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = CCTNode<NodeData>*;
        using pointer = CCTNode<NodeData>*;
        using reference = CCTNode<NodeData>&;

        explicit PostOrderIterator(CCTNode<NodeData>* node = nullptr) : currentNode(node) {
            if (node != nullptr) {
                stack.push(std::make_pair(node, false));
                exploreUntilLeftmostChild();
            }
        }

        CCTNode<NodeData>* operator*() const { return currentNode; }
        CCTNode<NodeData>* operator->() { return currentNode; }

        PostOrderIterator& operator++() { // Prefix increment
            if (this->stack.empty()) {
                this->currentNode = nullptr;
                return *this;
            }
            this->stack.pop();
            if (this->stack.empty()) {
                this->currentNode = nullptr;
                return *this;
            }
            exploreUntilLeftmostChild();
            return *this;
        }
        PostOrderIterator operator++(int) { // Postfix increment
            PostOrderIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const PostOrderIterator &a, const PostOrderIterator &b) { return a.currentNode == b.currentNode; }
        friend bool operator!=(const PostOrderIterator &a, const PostOrderIterator &b) { return a.currentNode != b.currentNode; }
    };

    /**
     * @brief Iterator for traversal from a node to the root of the tree.
     */
    class PathToRootIterator {
    private:
        /**
         * @brief The current node that is pointed to by the iterator.
         */
        CCTNode<NodeData>* currentNode;
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = CCTNode<NodeData>*;
        using pointer = CCTNode<NodeData>*;
        using reference = CCTNode<NodeData>&;

        explicit PathToRootIterator(CCTNode<NodeData>* node) : currentNode(node) {
            this->currentNode = node;
        }

        CCTNode<NodeData>* operator*() const { return currentNode; }
        CCTNode<NodeData>* operator->() { return currentNode; }

        PathToRootIterator& operator++() { // Prefix increment
            // TODO: decide if the endpoint should be nullptr or the auxiliary root
            this->currentNode = this->currentNode->parent;
            return *this;
        }
        PathToRootIterator operator++(int) { // Postfix increment
            PathToRootIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const PathToRootIterator& a, const PathToRootIterator& b) { return a.currentNode == b.currentNode; };
        friend bool operator!=(const PathToRootIterator& a, const PathToRootIterator& b) { return a.currentNode != b.currentNode; };
    };

    /**
     * @brief Iterator for traversing the tree in level-order.
     */
    class LevelOrderIterator {
    private:
        /**
         * @brief The current node that is pointed to by the iterator.
         */
        CCTNode<NodeData>* currentNode;
        /**
         * @brief the queue used for the BFS traversal.
         */
        std::deque<CCTNode<NodeData>*> queue;
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = CCTNode<NodeData>;
        using pointer = CCTNode<NodeData>*;
        using reference = CCTNode<NodeData>&;

        explicit LevelOrderIterator(CCTNode<NodeData>* node = nullptr) : currentNode(node) {
            this->currentNode = node;
            if (this->currentNode) {
                this->queue.push_back(this->currentNode);
            }
        }

        CCTNode<NodeData>* operator*() const { return currentNode; }
        CCTNode<NodeData>* operator->() { return currentNode; }

        LevelOrderIterator& operator++() { // Prefix increment
            if (this->queue.empty()) {
                this->currentNode = nullptr;
                return *this;
            }
            auto *node = this->queue.front();
            this->queue.pop_front();
            // Add children to the queue
            for (auto& [_, child] : node->children) {
                this->queue.push_back(child.get());
            }
            // Update the current node
            this->currentNode = this->queue.empty() ? nullptr : this->queue.front();
            return *this;
        }
        LevelOrderIterator operator++(int) { // Postfix increment
            LevelOrderIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const LevelOrderIterator& a, const LevelOrderIterator& b) { return a.currentNode == b.currentNode; }
        friend bool operator!=(const LevelOrderIterator& a, const LevelOrderIterator& b) { return a.currentNode != b.currentNode; }
    };

    using defaultIterator = PreOrderIterator;
    /**
     * @brief The begining iterator for the tree starting in the root and moving forward in pre-order fashion.
     * @return The iterator pointing to the root of the tree.
     */
    defaultIterator begin() { return defaultIterator(this->root.get()); }
    /**
     * @brief The begining iterator for the tree starting in the specified node and moving forward in pre-order fashion.
     * @param root the root of the (sub)tree to traverse
     * @return The iterator pointing to the specified node treating it as the root of the (sub)tree.
     */
    defaultIterator begin(CCTNode<NodeData>* root) { return defaultIterator(root); }
    /**
     * @brief The end iterator for the tree.
     * @return the end iterator for the tree.
     */
    defaultIterator end() { return defaultIterator(nullptr); }

    /**
     * @brief The begining iterator for the tree starting in the root and moving forward in pre-order fashion.
     * @return The pre-order iterator pointing to the root of the tree.
     */
    PreOrderIterator preOrderBegin() { return PreOrderIterator(this->root.get()); }
    /**
    * @brief The begining iterator for the tree starting in the specified node and moving forward in pre-order fashion.
    * @param root the root of the (sub)tree to traverse
    * @return The iterator pointing to the specified node treating it as the root of the (sub)tree.
    */
    PreOrderIterator preOrderBegin(CCTNode<NodeData>* root) { return PreOrderIterator(root); }
    /**
     * @brief The end of pre-order iterator for the tree.
     * @return the end of pre-order iterator for the tree.
     */
    PreOrderIterator preOrderEnd() { return PreOrderIterator(nullptr); }

    /**
     * @brief The begining iterator for the tree starting in the root and moving forward in post-order fashion.
     * @return The pre-order iterator pointing to the root of the tree.
     */
    PostOrderIterator postOrderBegin() { return PostOrderIterator(this->root.get()); }
    /**
     * @brief The begining iterator for the tree starting in the specified node and moving forward in post-order fashion.
     * @param root the root of the (sub)tree to traverse
     * @return The iterator pointing to the specified node treating it as the root of the (sub)tree.
     */
    PostOrderIterator postOrderBegin(CCTNode<NodeData>* root) { return PostOrderIterator(root); }
    /**
     * @brief The end of post-order iterator for the tree.
     * @return the end of post-order iterator for the tree.
     */
    PostOrderIterator postOrderEnd() { return PostOrderIterator(nullptr); }

    /**
     * @brief The begining iterator for the tree starting in the root and moving forward in level-order fashion.
     * @return The level-order iterator pointing to the root of the tree.
     */
    LevelOrderIterator levelOrderBegin() { return LevelOrderIterator(this->root.get()); }
     /**
     * @brief The begining iterator for the tree starting in the specified node and moving forward in level-order fashion.
     * @param root the root of the (sub)tree to traverse
     * @return The iterator pointing to the specified node treating it as the root of the (sub)tree.
     */
    LevelOrderIterator levelOrderBegin(CCTNode<NodeData>* root) { return LevelOrderIterator(root); }
    /**
     * @brief The end of level-order iterator for the tree.
     * @return the end of level-order iterator for the tree.
     */
    LevelOrderIterator levelOrderEnd() { return LevelOrderIterator(nullptr); }

    /**
     * @brief The begining iterator for the tree starting in the specified node and moving backward to the root of the tree.
     * @param node the node from which to travers back to the root.
     * @return The iterator pointing to the specified node.
     */
    PathToRootIterator pathToRootBegin(CCTNode<NodeData>* node) { return PathToRootIterator(node); }
    /**
     * @brief The end of path-to-root iterator for the tree (the root node).
     * @return the end of path-to-root iterator for the tree (the root node).
     */
    PathToRootIterator pathToRootEnd() { return PathToRootIterator(nullptr); }

protected:
    void pruneSubtree_rec(CCTNode<NodeData> *root, const int long long threshold);
    /**
     * @brief prune a subtree from its leaves according to the specified threashold.
     * @param root the node representing the root of the subtree to prune
     * @param threshold the threashold frequency of occurence of the function in a context
     */
    void pruneSubtree(CCTNode<NodeData> *root, const int long long threshold);

    class TreeInfo {
    public:
        std::vector<CCTNode<NodeData>*> postOrderNodes{};
        std::vector<int> leftMostDescendants{};
        std::vector<int> keyRoots{};
        long long int nodesCnt = 0;
    };

    std::unique_ptr<TreeInfo> getTreeInfo();


private:
    friend class boost::serialization::access;

    /**
     * @brief Save method used for serialization of the tree instance. Should not be called directly.
     * @tparam Archive the type of the archive to serialize into
     * @param ar the archive to serialize into
     * @param version the version of the serialization format
     */
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const {
        // Serialize root pointer - this will trigger serialization of the entire tree
        ar & BOOST_SERIALIZATION_NVP(root);
        ar & BOOST_SERIALIZATION_NVP(currentNode);
        ar & BOOST_SERIALIZATION_NVP(processName);
        ar & BOOST_SERIALIZATION_NVP(pid);
        ar & BOOST_SERIALIZATION_NVP(tid);
        ar & BOOST_SERIALIZATION_NVP(functionNameToIdMap);
        ar & BOOST_SERIALIZATION_NVP(functionIdToNameMap);
    }

    /**
     * @brief Load method used for serialization of the tree instance. Should not be called directly.
     * @tparam Archive the type of the archive to serialize into
     * @param ar the archive to serialize into
     * @param version the version of the serialization format
     */
    template<class Archive>
    void load(Archive & ar, const unsigned int version) {
        this->root = nullptr;
        this->currentNode = nullptr;
        ar & BOOST_SERIALIZATION_NVP(root);
        ar & BOOST_SERIALIZATION_NVP(currentNode);
        ar & BOOST_SERIALIZATION_NVP(processName);
        ar & BOOST_SERIALIZATION_NVP(pid);
        ar & BOOST_SERIALIZATION_NVP(tid);
        ar & BOOST_SERIALIZATION_NVP(functionNameToIdMap);
        ar & BOOST_SERIALIZATION_NVP(functionIdToNameMap);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER() // Allows to split default serialization function into save and load functions
};

template<class NodeData>
CCTree<NodeData>::CCTree() {
    this->root = std::make_unique<CCTNode<NodeData>>(AUXILIARY_ROOT_ID);
    this->functionNameToIdInsert(std::string{AUXILIARY_ROOT_NAME});
    this->currentNode = root.get();
}

template<class NodeData>
CCTree<NodeData>::~CCTree() {
    this->root = nullptr;
    this->currentNode = nullptr;
}

template<class NodeData>
CCTNode<NodeData>* CCTree<NodeData>::getCurrentNode() {
    return this->currentNode;
}

template<class NodeData>
void CCTree<NodeData>::setCurrentNode(CCTNode<NodeData> *node) {
    this->currentNode = node;
}

template<class NodeData>
CCTNode<NodeData> * CCTree<NodeData>::getRootNode() {
    return this->root.get();
}

template<class NodeData>
const CCTNode<NodeData> * CCTree<NodeData>::getRootNode() const {
    return this->root.get();
}

template<class NodeData>
CCTNode<NodeData>* CCTree<NodeData>::getParentOfCurrentNode() {
    return this->currentNode->parent;
}

template<class NodeData>
CCTNode<NodeData>* CCTree<NodeData>::getChildOfCurrentNode(const std::string &name) {
    auto [fId, found] = this->functionNameToId(name);
    return found ? this->currentNode->getChild(fId) : nullptr;
}
template<class NodeData>
CCTNode<NodeData>* CCTree<NodeData>::getChildOfCurrentNode(std::string_view name) {
    auto [fId, found] = this->functionNameToId(name);
    return found ? this->currentNode->getChild(fId) : nullptr;
}

template<class NodeData>
CCTNode<NodeData>* CCTree<NodeData>::addNewChildToCurrentNode(const std::string &name) {
    // Note: expactes that the node name does not exist in children yet
    auto fId = this->functionNameToIdInsert(name);
    return this->currentNode->addChild(std::make_unique<CCTNode<NodeData>>(fId, this->currentNode));
}

// template<class NodeData>
// void CCTree<NodeData>::removeNode(CCTNode<NodeData>* node) {
//     auto parent = node->parent;
//     parent->children.erase(node);
// }

template<class NodeData>
void CCTree<NodeData>::prune(const long long int threshold) {
    // Note: invalidate current node. It is assumed that the building of the tree is finished
    // The current node should not be removed by prunning since it is at the auxiliary root at the end.
    this->currentNode = nullptr;
    this->pruneSubtree(this->root.get(), threshold);
}

template<class NodeData>
std::pair<int, std::vector<Operation<CCTNode<NodeData>>>> CCTree<NodeData>::treeEditDistance(CCTree<NodeData> &other) {
    /**
     * This method implements the Zhang-sasha tree edit distnace algorithm
     * from following article https://dl.acm.org/doi/abs/10.1145/1133255.1134012.
     * Its implementation was inspired by the Python Zhang-Shasha Tree Edit Distance implementation
     * from https://github.com/timtadh/zhang-shasha/.
     */

    // Edit operations cost definitions
    auto removeCost = [](CCTNode<NodeData>* n) { return 1; };
    auto insertCost = [](CCTNode<NodeData>* n) { return 1; };
    auto updateCost = [](CCTNode<NodeData>* a, CCTNode<NodeData>* b) {return a->functionId == b->functionId ? 0 : 1;};


    std::unique_ptr<TreeInfo> thisTreeInfo = this->getTreeInfo();
    std::unique_ptr<TreeInfo> otherTreeInfo = other.getTreeInfo();

    std::vector<std::vector<int>> treeDistance(thisTreeInfo->nodesCnt, std::vector<int>(otherTreeInfo->nodesCnt, 0));
    std::vector<std::vector<std::vector<Operation<CCTNode<NodeData>>>>> operations(thisTreeInfo->nodesCnt, std::vector<std::vector<Operation<CCTNode<NodeData>>>>(otherTreeInfo->nodesCnt));

    for (int i : thisTreeInfo->keyRoots) {
        for (int j : otherTreeInfo->keyRoots) {
            int m = i - thisTreeInfo->leftMostDescendants[i] + 2;
            int n = j - otherTreeInfo->leftMostDescendants[j] + 2;
            std::vector fd(m, std::vector(n, 0));
            std::vector partialOperations(m, std::vector(n, std::vector<Operation<CCTNode<NodeData>>>()));

            int ioff = thisTreeInfo->leftMostDescendants[i] - 1;
            int joff = otherTreeInfo->leftMostDescendants[j] - 1;

            for (int x = 1; x < m; ++x) {
                auto node = thisTreeInfo->postOrderNodes[x + ioff];
                fd[x][0] = fd[x - 1][0] + removeCost(node);
                partialOperations[x][0] = partialOperations[x - 1][0];
                partialOperations[x][0].emplace_back(Operation<CCTNode<NodeData>>::REMOVE, node);
            }

            for (int y = 1; y < n; ++y) {
                auto node = otherTreeInfo->postOrderNodes[y + joff];
                fd[0][y] = fd[0][y - 1] + insertCost(node);
                partialOperations[0][y] = partialOperations[0][y - 1];
                partialOperations[0][y].emplace_back(Operation<CCTNode<NodeData>>::INSERT, nullptr, node);
            }

            for (int x = 1; x < m; ++x) {
                for (int y = 1; y < n; ++y) {
                    // x+ioff in the fd table corresponds to the same node as x in
                    // the treedists table (same for y and y+joff)
                    auto node1 = thisTreeInfo->postOrderNodes[x + ioff];
                    auto node2 = otherTreeInfo->postOrderNodes[y + joff];
                    // only need to check if x is an ancestor of i and y is an ancestor of j
                    if (thisTreeInfo->leftMostDescendants[i] == thisTreeInfo->leftMostDescendants[x + ioff] &&
                        otherTreeInfo->leftMostDescendants[j] == otherTreeInfo->leftMostDescendants[y + joff]) {
                        //                   +------
                        //                   |  δ(l(i1)..i-1, l(j1)..j) + removeCost
                        // δ(F1 , F2 ) = min-+  δ(l(i1)..i , l(j1)..j-1) + insertCost
                        //                   |  δ(l(i1)..i-1, l(j1)..j-1) + updateCost
                        //                   +------
                        std::vector<int> costs = {
                            fd[x - 1][y] + removeCost(node1),
                            fd[x][y - 1] + insertCost(node2),
                            fd[x - 1][y - 1] + updateCost(node1, node2)
                        };
                        auto minIt = std::min_element(costs.begin(), costs.end());
                        int minIndex = std::distance(costs.begin(), minIt);
                        fd[x][y] = *minIt;

                        if (minIndex == 0) {
                            partialOperations[x][y] = partialOperations[x - 1][y];
                            partialOperations[x][y].emplace_back(Operation<CCTNode<NodeData>>::REMOVE, node1);
                        } else if (minIndex == 1) {
                            partialOperations[x][y] = partialOperations[x][y - 1];
                            partialOperations[x][y].emplace_back(Operation<CCTNode<NodeData>>::INSERT, nullptr, node2);
                        } else {
                            typename Operation<CCTNode<NodeData>>::Type op_type = (fd[x][y] == fd[x - 1][y - 1])
                                                                             ? Operation<CCTNode<NodeData>>::MATCH
                                                                             : Operation<CCTNode<NodeData>>::UPDATE;
                            partialOperations[x][y] = partialOperations[x - 1][y - 1];
                            partialOperations[x][y].emplace_back(op_type, node1, node2);
                        }

                        operations[x + ioff][y + joff] = partialOperations[x][y];
                        treeDistance[x + ioff][y + joff] = fd[x][y];
                    } else {
                        //                  +----
                        //                  |  δ(l(i1)..i-1, l(j1)..j) + removeCost
                        // δ(F1 , F2) = min-+  δ(l(i1)..i , l(j1)..j-1) + insertCost
                        //                  |  δ(l(i1)..l(i)-1, l(j1)..l(j)-1 + treedist(i1,j1)
                        //                  +----
                        int p = thisTreeInfo->leftMostDescendants[x + ioff] - 1 - ioff;
                        int q = otherTreeInfo->leftMostDescendants[y + joff] - 1 - joff;
                        std::vector<int> costs = {
                            fd[x - 1][y] + removeCost(node1),
                            fd[x][y - 1] + insertCost(node2),
                            fd[p][q] + treeDistance[x + ioff][y + joff]
                        };
                        auto min_it = std::min_element(costs.begin(), costs.end());
                        int min_index = std::distance(costs.begin(), min_it);
                        fd[x][y] = *min_it;

                        if (min_index == 0) {
                            partialOperations[x][y] = partialOperations[x - 1][y];
                            partialOperations[x][y].emplace_back(Operation<CCTNode<NodeData>>::REMOVE, node1);
                        } else if (min_index == 1) {
                            partialOperations[x][y] = partialOperations[x][y - 1];
                            partialOperations[x][y].emplace_back(Operation<CCTNode<NodeData>>::INSERT, nullptr, node2);
                        } else {
                            partialOperations[x][y] = partialOperations[p][q];
                            partialOperations[x][y].insert(partialOperations[x][y].end(),
                                                     operations[x + ioff][y + joff].begin(),
                                                     operations[x + ioff][y + joff].end());
                        }
                    }
                }
            }
        }
    }

    return {treeDistance.back().back(), operations.back().back()};
}

template<class NodeData>
void CCTree<NodeData>::merge(CCTree<NodeData> &&other) {
    other.root->remapFunctionId(other.functionIdToNameMap, *this);
    getRootNode()->merge(std::forward<std::unique_ptr<CCTNode<NodeData>>>(other.root));
}

template<class NodeData>
void CCTree<NodeData>::pruneSubtree_rec(CCTNode<NodeData> *root, const long long int threshold) {
    std::erase_if(root->children, [this, threshold](auto &item) {
        auto &[_, child] = item;
        if (child == nullptr) {
            return false;
        }
        this->pruneSubtree_rec(child, threshold);
        return child->children.empty() && child->data->getInvocationFrequency() < threshold;
    });
}

/*
 * Prune the CHILDREN of root
 */
template<class NodeData>
void CCTree<NodeData>::pruneSubtree(CCTNode<NodeData> *root, const long long int threshold) {
    if (root == nullptr) {
        return;
    }
    this->pruneSubtree_rec(root, threshold);

    auto *parent = root->parent;
    if (parent != nullptr) {
        parent->removeChild(root->functionId);
    }
}

template<class NodeData>
std::unique_ptr<typename CCTree<NodeData>::TreeInfo> CCTree<NodeData>::getTreeInfo() {
    std::map<int, int> keyRootsMap{};
    std::map<CCTNode<NodeData>*, int> leftMostDescendantsMap{};
    auto treeInfo = std::make_unique<TreeInfo>();

    int index = 0;
    for (auto it = this->postOrderBegin(); it != this->postOrderEnd(); ++it) {
        auto *node = *it;
        treeInfo->nodesCnt += 1;
        treeInfo->postOrderNodes.push_back(node);

        int leftMostDescendantIndexOfCurrentNode;
        if (it->children.empty()) { // Is leaf
            leftMostDescendantIndexOfCurrentNode = index;
            // Propagate the information about lefmost node to its parents that do not already have leftmost node
            for (auto backIt = this->pathToRootBegin(node->parent); backIt != this->pathToRootEnd(); ++backIt) {
                auto *predecessorNode = *backIt;
                auto [_, wasInserted] = leftMostDescendantsMap.emplace(predecessorNode,
                                                                       leftMostDescendantIndexOfCurrentNode);
                if (!wasInserted) {
                    // as soon as a paren has a leftmost descendant stop (the others have to be assigned as well)
                    break;
                }
            }
        } else { // has children
            // the leftmost node had to be encountered already - take it from the helper map
            leftMostDescendantIndexOfCurrentNode = leftMostDescendantsMap.at(node);
        }
        treeInfo->leftMostDescendants.push_back(leftMostDescendantIndexOfCurrentNode);
        auto [currentKeyRoot, wasInserted] = keyRootsMap.emplace(leftMostDescendantIndexOfCurrentNode, index);
        if (!wasInserted) {
            currentKeyRoot->second = index;
        }
        index++;
    }
    for (auto [_, keyRootIndex]: keyRootsMap) {
        treeInfo->keyRoots.push_back(keyRootIndex);
    }
    std::sort(treeInfo->keyRoots.begin(), treeInfo->keyRoots.end());
    return std::forward(treeInfo);
}

template<class NodeData>
std::string CCTree<NodeData>::toString() const {
    std::stringstream sstream;
    std::string offset;
    const std::string offsetStr = " | ";
    std::stack<CCTNode<NodeData>*> stack;

    stack.push(this->root.get());

    while(!stack.empty()) {
        CCTNode<NodeData>* currentNode = stack.top();
        stack.pop();
        if (currentNode == nullptr) {
            for (auto& _ : offsetStr) {
                offset.pop_back();
            }
            continue;
        }

        sstream << offset << this->functionIdToName(currentNode->functionId) << "\n";

        stack.emplace(nullptr);
        offset += " | ";
        for (auto &[_, child] : currentNode->children) {
            stack.emplace(child.get());
        }
    }
    return sstream.str();
}

template<class NodeData>
bool CCTree<NodeData>::isEmpty() const {
    return currentNode == root and this->root->children.empty();
}

template<class NodeData>
long long CCTree<NodeData>::getNumberOfNodes() {
    long long cnt = 0;
    for (auto it = this->levelOrderBegin(); it != this->levelOrderEnd(); ++it) {
        ++cnt;
    }
    return cnt;
}

template<class NodeData>
long long CCTree<NodeData>::getNumberOfFunctions() {
    return functionIdToNameMap.size();
}

template<class NodeData>
long long CCTree<NodeData>::getMaximumInvocations() {
    long long max = -1;
    for (auto it = this->begin(); it != this->end(); ++it) {
        auto* node = *it;
        long long invocationFrequency = node->data->getInvocationFrequency();
        if (invocationFrequency > max) {
            max = invocationFrequency;
        }
    }
    return max;
}


template<class NodeData>
void CCTNode<NodeData>::remapFunctionId(const std::vector<std::string> &oldMap, CCTree<NodeData> &newTree) {
    if (this->functionId >= oldMap.size()) {
        std::cerr << "[E]: Function id " << this->functionId << " of CCTNode not found in tree's map!" << std::endl;
        return;
    }
    this->functionId = newTree.functionNameToIdInsert(oldMap[this->functionId]);

    auto oldChildren{std::forward<std::unordered_map<functionIdType, std::unique_ptr<CCTNode>>>(this->children)};
    this->children.clear();
    for (auto &&[_, child] : oldChildren) {
        child->remapFunctionId(oldMap, newTree);
        this->addChild(std::forward<std::unique_ptr<CCTNode<NodeData>>>(child));
        child = nullptr;
    }
}

// CCForest
template <class NodeData>
class CCForest {
private:
    /**
     * @brief Map of the trees in the forest based on the process id (pid) and thread id (tid) of the tree.
     */
    std::unordered_map<std::pair<int, int>, std::unique_ptr<CCTree<NodeData>>, PairHash> forestMap = {};
public:
    using valueType = NodeData;

    /**
     * @brief Creates empty forest.
     */
    CCForest() = default;
    /**
     * @brief Deletes the whole forest.
     */
    ~CCForest();

    /**
     * @brief Retrieve the tree with specified pid and tid.
     * @param pid process identifier of the tree
     * @param tid thread identifier of the tree
     * @return the tree with specified process and thread identifiers
     */
    CCTree<NodeData>* getTree(int pid, int tid);
    /**
     * @brief Adds the specified tree to the forest. The tree has to have pid and tid set
     * @param tree tree to be added to the forest (with pid and tid set)
     */
    void addTree(std::unique_ptr<CCTree<NodeData>> &&tree);
    /**
     * @brief Creates a new tree and adds it to the forest.
     * @param pid process id of the new tree
     * @param tid thread id of the new tree
     * @param processName name of the process of the new tree
     * @return the newly created tree
     */
    CCTree<NodeData>* addNewTree(int pid, int tid, const std::string& processName= "");

    /**
     * @brief Runs pruning algorithm over all trees in the forest.
     * @param threshold the threshold of occurrence frequency of functions within their contexts
     */
    void prune(const long long int threshold);

    /**
     * @brief Forms a string representation of the forest.
     * @return the string representation of the forest
     */
    std::string toString() const;

    /**
     * @brief checks if the forest has any tries in it.
     * @return true if the forest is empty, false otherwise
     */
    bool isEmpty() const;


    /**
     * @brief Retrieves number of nodes in the forest structure. Sum of all sub-trees.
     * @return number of nodes in the forest
     */
    long long getNumberOfNodes();

    /**
     * @brief Retrieves number of unique functions in the forest structure. Sum of all sub-trees
     * @return number of unique functions in the forest structure
     */
    long long getNumberOfFunctions();

    /**
     * @brief Retrieves maximal number of invocations of a node in the forest-- the maximal number
     * of calls a function in the forest has.
     * @return number of maximal invocations of a function from the forest
     */
    long long getMaximumInvocations();

    /**
     * @brief Iterator over trees in the forest.
     */
    class TreeIterator {
    private:
        /**
         * @brief Stores the keys of the map in sorted order for deterministic iteration.
         */
        std::vector<std::pair<int, int>> keys{};
        /**
         * @brief The forest map that is being iterated over.
         */
        const std::unordered_map<std::pair<int, int>, std::unique_ptr<CCTree<NodeData>>, PairHash>* forestMap = nullptr;
        /**
         * @brief the index of the current key in the keys vector. Specifies the current tree in the forest.
         */
        size_t currentKeyIndex{};
        /**
         * @brief the current pair from the forest map that is being iterated over.
         */
        std::pair<std::pair<int, int>, CCTree<NodeData>*> currentPair;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<std::pair<int, int>, CCTree<NodeData>*>*;
        using pointer = std::pair<std::pair<int, int>, CCTree<NodeData>*>*;
        using reference = std::pair<std::pair<int, int>, CCTree<NodeData>*>&;


        explicit TreeIterator(const std::unordered_map<std::pair<int, int>, std::unique_ptr<CCTree<NodeData>>, PairHash>* map, bool isEnd = false) : forestMap(map) {
            size_t mapSize = map->size();
            if (isEnd) {
                this->currentKeyIndex = map != nullptr ? mapSize : 0;
                return;
            }

            // Sort the keys for deterministic iteration
            this->keys.reserve(mapSize);
            for (const auto& [key, _] : *map) {
                this->keys.push_back(key);
            }
            std::sort(this->keys.begin(), this->keys.end());

            this->currentKeyIndex = 0;
            auto& currentKey = this->keys[this->currentKeyIndex];
            this->currentPair = std::make_pair(currentKey, this->forestMap->at(currentKey).get());
        }

        reference operator*() { return this->currentPair; }
        pointer operator->() { return &(this->currentPair); }

        TreeIterator& operator++() { // Prefix increment
            ++this->currentKeyIndex;
            if (this->currentKeyIndex >= this->keys.size()) {
                this->currentKeyIndex = this->keys.size();
                return *this;
            }
            auto& currentKey = this->keys[this->currentKeyIndex];
            this->currentPair = std::make_pair(currentKey, this->forestMap->at(currentKey).get());
            return *this;
        }
        TreeIterator operator++(int) { // Postfix increment
            TreeIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const TreeIterator& a, const TreeIterator& b) {
            return a.currentKeyIndex == b.currentKeyIndex and a.forestMap == b.forestMap;
        }
        friend bool operator!=(const TreeIterator& a, const TreeIterator& b) {
            return a.currentKeyIndex != b.currentKeyIndex or a.forestMap != b.forestMap;
        }
    };

    using defaultIterator = TreeIterator;
    /**
     * @brief Bigining iterator for the forest. Iteration is always deterministic over sorted order of tree keys.
     * @return an iterator pointing to the first tree in the forest
     */
    defaultIterator begin() { return defaultIterator(&(this->forestMap)); }
    /**
     * @brief End iterator for the forest. Iteration is always deterministic over sorted order of tree keys.
     * @return an iterator pointing to the end of the forest
     */
    defaultIterator end() { return defaultIterator(&(this->forestMap), true); }

    /**
     * @brief Comparison of the forest based on the structure of each tree. It uses the comparison of nodes
     * and thus only compares their function names and not the data they hold. Hence, it is allowed to missmatched
     * NodeData types.
     * @tparam U the type of the data in nodes of the tress of the other forest
     * @param other the other forest
     * @return true if the forests have same trees with the same structure, false otherwise
     */
    template <typename U>
    bool operator==(CCForest<U>& other) {
        auto it1 = this->begin();
        auto it2 = other.begin();
        bool areEqual = true;
        while (it1 != this->end()) {
            if (it2 == other.end()) {
                areEqual = false;
                break;
            }

            auto* tree1 = it1->second;
            auto* tree2 = it2->second;
            if (*tree1 != *tree2) {
                areEqual = false;
                break;
            }
            ++it1;
            ++it2;
        }
        return areEqual;
    }

    /**
     * @brief Comparison of the forest based on the structure of each tree. It uses the comparison of nodes
     * and thus only compares their function names and not the data they hold. Hence, it is allowed to missmatched
     * NodeData types.
     * @tparam U the type of the data in nodes of the tress of the other forest
     * @param other the other forest
     * @return true if the forests have same trees with the same structure, false otherwise
     */
    template <typename U>
    bool operator!=(CCForest<U>& other) {
        return !(*this == other);
    }

private:
    friend class boost::serialization::access;

    /**
     * @brief The function responsible for serialization and deserialization of the forest.
     * @tparam Archive the archive type
     * @param ar the archive to serialize to or deserialize from
     * @param version the version of the serialization
     */
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_NVP(this->forestMap);
    }
};

template<class NodeData>
CCForest<NodeData>::~CCForest() {
}

template<class NodeData>
CCTree<NodeData>* CCForest<NodeData>::getTree(int pid, int tid) {
    const auto key = std::make_pair(pid, tid);
    if (const auto it = this->forestMap.find(key); it != this->forestMap.end()) {
        return it->second.get();
    }
    return nullptr;
}

template<class NodeData>
void CCForest<NodeData>::addTree(std::unique_ptr<CCTree<NodeData>> &&tree) {
    const auto key = std::make_pair(tree->pid, tree->tid);
    if (const auto it = this->forestMap.find(key); it != this->forestMap.end()) {
        std::cerr << "[W]: A tree with specified pid and tid already exists! "
                  << "Will not add a tree with the same process identification to the forest." << std::endl;
        return;
    }
    this->forestMap.emplace(key, std::forward<std::unique_ptr<CCTree<NodeData>>>(tree));
}

template<class NodeData>
CCTree<NodeData>* CCForest<NodeData>::addNewTree(int pid, int tid, const std::string& processName) {
    // Note: expects the tree to be new (pid and tid not in the forestMap yet)
    auto* tree = this->forestMap.emplace(std::make_pair(pid, tid), std::make_unique<CCTree<NodeData>>()).first->second.get();
    tree->processName = processName;
    tree->pid = pid;
    tree->tid = tid;
    return tree;
}

template<class NodeData>
void CCForest<NodeData>::prune(const long long int threshold) {
    for (auto& [treeidm, treePtr] : this->forestMap) {
       treePtr->prune(threshold);
    }
}

template<class NodeData>
std::string CCForest<NodeData>::toString() const {
    std::stringstream sstream;
    for (auto& [id, tree] : forestMap) {
        sstream << "PID: " << id.first << " TID: " << id.second << " Process Name: " << tree->processName << "\n"
                << tree->toString() << "\n";
    }
    return sstream.str();
}

template<class NodeData>
bool CCForest<NodeData>::isEmpty() const {
    return this->forestMap.empty();
}

template<class NodeData>
long long CCForest<NodeData>::getNumberOfNodes() {
    long long cnt = 0;
    for (auto& [id, tree] : forestMap) {
        cnt += tree->getNumberOfNodes();
    }
    return cnt;
}

template<class NodeData>
long long CCForest<NodeData>::getNumberOfFunctions() {
    std::unordered_set<std::string> uniqueFunctionNames = {};
    for (const auto&[_, tree] : forestMap) {
        for (const auto &name : tree->functionIdToNameMap) {
            uniqueFunctionNames.insert(name);
        }
    }
    return uniqueFunctionNames.size();
}

template<class NodeData>
long long CCForest<NodeData>::getMaximumInvocations() {
    long long max = -1;
    for (auto& [id, tree] : forestMap) {
        long long current_number_of_invocations = tree->getMaximumInvocations();
        if (current_number_of_invocations > max) {
            max = current_number_of_invocations;
        }
    }
    return max;
}


#endif // CCT_HPP
