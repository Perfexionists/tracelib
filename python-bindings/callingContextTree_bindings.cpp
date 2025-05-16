#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tracelib/callingContextTree.hpp"
#include "tracelib/perunPin.hpp"
#include "tracelib/perfFolded.hpp"
#include "tracelib/perunSystemTap.hpp"
#include "tracelib/utils.hpp"


#define NODE_DATA_TYPES PerunPinNodeData, PerunSystemTapNodeData, PerfFoldedNodeData


namespace py = pybind11;


template<class Type>
void bindCCTClassesForType(py::module& m, const std::string& typeName) {

    //CCNode<NodeData>
    using CCTNodeClass = CCTNode<Type>;
    std::string className = "CCTNode_" + typeName;
    py::class_<CCTNodeClass>(m, className.c_str())
        .def(py::init<>())
        .def(py::init<const std::string&, CCTNodeClass*>())
        .def("__str__", &CCTNodeClass::toString)
        .def("__ne__", [](CCTNodeClass& a, CCTNodeClass& b) {return a != b; })
        .def("__eq__", [](CCTNodeClass& a, CCTNodeClass& b) {return a == b; })
        .def_readonly("function_name", &CCTNodeClass::functionName)
        .def_readonly("data", &CCTNodeClass::data, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def_readonly("parent", &CCTNodeClass::parent, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def_readonly("children", &CCTNodeClass::children, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("get_child", &CCTNodeClass::getChild,
            py::arg("child_name"),
            py::return_value_policy::reference_internal, py::keep_alive<0,1>())
    ;

    //CCTree<NodeData>
    using CCTreeClass = CCTree<Type>;
    className = "CCTree_" + typeName;
    py::class_<CCTreeClass>(m, className.c_str())
        .def(py::init<>())
        .def("__str__", &CCTreeClass::toString)
        .def("__ne__", [](CCTreeClass& a, CCTreeClass& b) {return a != b; })
        .def("__eq__", [](CCTreeClass& a, CCTreeClass& b) {return a == b; })
        .def("__iter__", [](CCTreeClass &tree) {
            return py::make_iterator(tree.begin(), tree.end());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("__iter__",  [](CCTreeClass &tree, CCTNode<Type>* root) {
            return py::make_iterator(tree.begin(root), tree.end());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("pre_order", [](CCTreeClass &tree) {
            return py::make_iterator(tree.preOrderBegin(), tree.preOrderEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("pre_order", [](CCTreeClass &tree, CCTNode<Type>* root) {
            return py::make_iterator(tree.preOrderBegin(root), tree.preOrderEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("post_order", [](CCTreeClass &tree) {
            return py::make_iterator(tree.postOrderBegin(), tree.postOrderEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("post_order", [](CCTreeClass &tree, CCTNode<Type>* root) {
            return py::make_iterator(tree.postOrderBegin(root), tree.postOrderEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("level_order", [](CCTreeClass &tree) {
            return py::make_iterator(tree.levelOrderBegin(), tree.levelOrderEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("level_order", [](CCTreeClass &tree, CCTNode<Type>* root) {
            return py::make_iterator(tree.levelOrderBegin(root), tree.levelOrderEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("path_to_root", [](CCTreeClass &tree, CCTNode<Type>* node) {
            return py::make_iterator(tree.pathToRootBegin(node), tree.pathToRootEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def_readonly("process_name", &CCTreeClass::processName)
        .def_readonly("pid", &CCTreeClass::pid)
        .def_readonly("tid", &CCTreeClass::tid)
        .def("is_empty", &CCTreeClass::isEmpty)
        .def("get_current_node", &CCTreeClass::getCurrentNode, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("get_root_node", &CCTreeClass::getRootNode, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("get_parent_of_current_node", &CCTreeClass::getParentOfCurrentNode, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("get_child_of_current_node", &CCTreeClass::getChildOfCurrentNode,
            py::arg("child_name"),
            py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("prune", &CCTreeClass::prune, py::arg("threshold"))
        .def("edit_distance", &CCTreeClass::treeEditDistance, py::arg("other"), py::return_value_policy::take_ownership)
        .def("get_number_of_nodes", &CCTreeClass::getNumberOfNodes)
        .def("get_number_of_functions", &CCTreeClass::getNumberOfFunctions)
        .def("get_maximum_invocations", &CCTreeClass::getMaximumInvocations)
    ;

    //CCForest<NodeData>
    using CCForestClass = CCForest<Type>;
    className = "CCForest_" + typeName;
    py::class_<CCForestClass>(m, className.c_str())
        .def(py::init<>())
        .def("__str__", &CCForestClass::toString)
        .def("__ne__", [](CCForestClass& a, CCForestClass& b) {return a != b; })
        .def("__eq__", [](CCForestClass& a, CCForestClass& b) {return a == b; })
        .def("__iter__", [](CCForestClass &tree) {
            return py::make_iterator(tree.begin(), tree.end());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("is_empty", &CCForestClass::isEmpty)
        .def("get_tree", &CCForestClass::getTree,
            py::arg("pid"),
            py::arg("tid"),
            py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("prune", &CCForestClass::prune, py::arg("threshold"))
        .def("get_number_of_nodes", &CCForestClass::getNumberOfNodes)
        .def("get_number_of_functions", &CCForestClass::getNumberOfFunctions)
        .def("get_maximum_invocations", &CCForestClass::getMaximumInvocations)
    ;

    // Operation used for tree edit distance
    // Note: there is no need for CCGNode suport since the edit distance is not implemented over CG types
    using OperationClass = Operation<CCTNode<Type>>;
    className = "Operation_" + typeName;
    py::class_<OperationClass>(m, className.c_str())
        .def_readonly("arg1", &OperationClass::arg1)
        .def_readonly("arg2", &OperationClass::arg2)
        .def("type", [](OperationClass &op) {
            switch (op.type) {
                case OperationClass::Type::MATCH: return std::string("MATCH");
                case OperationClass::Type::REMOVE: return std::string("REMOVE");
                case OperationClass::Type::UPDATE: return std::string("UPDATE");
                case OperationClass::Type::INSERT: return std::string("INSERT");
                default: return std::string("UNKNOWN");
            }
        })
        .def("__str__", &OperationClass::toString)
        .def("__repr__", &OperationClass::toString)
    ;
}

template<class... Types>
void bindCCTClassesForAllTypes(py::module_& m) {
    (bindCCTClassesForType<Types>(m, std::string(py::type_id<Types>())), ...);
}

void cctBindings(py::module& m) {
    m.doc() = "pybind11 module for tracelib library";
    bindCCTClassesForAllTypes<NODE_DATA_TYPES>(m);
}
