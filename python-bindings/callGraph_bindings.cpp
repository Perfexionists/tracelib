#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tracelib/callGraph.hpp"
#include "tracelib/perunPin.hpp"
#include "tracelib/perfFolded.hpp"
#include "tracelib/perunSystemTap.hpp"


#define NODE_DATA_TYPES PerunPinNodeData, PerunSystemTapNodeData, PerfFoldedNodeData


namespace py = pybind11;


template<class Type>
void bindCGClassesForType(py::module& m, const std::string& typeName) {

    //CCGNode<NodeData>
    using CCGNodeClass = CCGNode<Type>;
    std::string className = "CCGNode_" + typeName;
    py::class_<CCGNodeClass>(m, className.c_str())
        .def(py::init<>())
        .def(py::init<const std::string&>())
        .def("__str__", &CCGNodeClass::toString)
        .def("__ne__", [](CCGNodeClass& a, CCGNodeClass& b) {return a != b; })
        .def("__eq__", [](CCGNodeClass& a, CCGNodeClass& b) {return a == b; })
        .def_readonly("function_name", &CCGNodeClass::functionName)
        .def_readonly("data", &CCGNodeClass::data, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
    ;

    //CCGraph<NodeData>
    using CCGraphClass = CCGraph<Type>;
    className = "CCGraph_" + typeName;
    py::class_<CCGraphClass>(m, className.c_str())
        .def(py::init<>())
        .def("__str__", &CCGraphClass::toString)
        .def("__ne__", [](CCGraphClass& a, CCGraphClass& b) {return a != b; })
        .def("__eq__", [](CCGraphClass& a, CCGraphClass& b) {return a == b; })
        .def("__iter__", [](CCGraphClass &graph) {
            return py::make_iterator(graph.begin(), graph.end());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("__iter__",  [](CCGraphClass &graph, CCGNode<Type>* root) {
            return py::make_iterator(graph.begin(root), graph.end());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("bfs", [](CCGraphClass &graph) {
            return py::make_iterator(graph.bfsBegin(), graph.bfsEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("bfs", [](CCGraphClass &graph, CCGNode<Type>* root) {
            return py::make_iterator(graph.bfsBegin(root), graph.bfsEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("dfs", [](CCGraphClass &graph) {
            return py::make_iterator(graph.dfsBegin(), graph.dfsEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("dfs", [](CCGraphClass &graph, CCGNode<Type>* root) {
            return py::make_iterator(graph.dfsBegin(root), graph.dfsEnd());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def_readonly("process_name", &CCGraphClass::processName)
        .def_readonly("pid", &CCGraphClass::pid)
        .def_readonly("tid", &CCGraphClass::tid)
        .def("get_node", &CCGraphClass::getNode)
        .def("get_current_node", &CCGraphClass::getCurrentNode)
        .def("get_child_of_current_node", &CCGraphClass::getChildOfCurrentNode)
        .def("is_empty", &CCGraphClass::isEmpty)
        .def("get_number_of_nodes", &CCGraphClass::getNumberOfNodes)
        .def("get_number_of_functions", &CCGraphClass::getNumberOfFunctions)
    ;

    //DCGraph<NodeData>
    using DCGraphClass = DCGraph<Type>;
    className = "DCGraph_" + typeName;
    py::class_<DCGraphClass>(m, className.c_str())
        .def(py::init<>())
        .def("__str__", &DCGraphClass::toString)
        .def("__ne__", [](DCGraphClass& a, DCGraphClass& b) {return a != b; })
        .def("__eq__", [](DCGraphClass& a, DCGraphClass& b) {return a == b; })
        .def("__iter__", [](DCGraphClass &multi_graph) {
            return py::make_iterator(multi_graph.begin(), multi_graph.end());
        }, py::return_value_policy::reference_internal, py::keep_alive<0,1>())
        .def("get_graph", &DCGraphClass::getGraph, py::return_value_policy::reference_internal)
        .def("is_empty", &DCGraphClass::isEmpty)
        .def("get_number_of_nodes", &DCGraphClass::getNumberOfNodes)
        .def("get_number_of_functions", &DCGraphClass::getNumberOfFunctions)
    ;
}

template<class... Types>
void bindCGClassesForAllTypes(py::module_& m) {
    (bindCGClassesForType<Types>(m, std::string(py::type_id<Types>())), ...);
}

void callGraphBindings(py::module& m) {
    bindCGClassesForAllTypes<NODE_DATA_TYPES>(m);
}
