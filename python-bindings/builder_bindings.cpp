#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tracelib/perfFolded.hpp"
#include "tracelib/perunPin.hpp"
#include "tracelib/perunSystemTap.hpp"
#include "tracelib/builder.hpp"
#include "tracelib/callingContextTree.hpp"
#include "tracelib/callGraph.hpp"

#define GRAPH_TYPES  CCTree, CCForest, CCGraph, DCGraph
#define GRAPH_TYPES_AS_STRING "CCTree", "CCForest", "CCGraph", "DCGraph"
#define NODE_DATA_TYPES PerunPinNodeData, PerunSystemTapNodeData, PerfFoldedNodeData

namespace py = pybind11;


template<template<class> class GraphType, class NodeData>
void bindBuilderForType(py::module& m, const std::string& GraphTypeName, const std::string& NodeDataTypeName) {

    //Builder<GraphType<NodeData>>
    using CCTBuilderClass = Builder<GraphType<NodeData>>;
    const std::string className = "Builder_" + GraphTypeName + "_" + NodeDataTypeName;
    py::class_<CCTBuilderClass>(m, className.c_str())
        .def(py::init<>())
        .def("build", &CCTBuilderClass::build,
            py::arg("graph"),
            py::arg("parser"),
            py::keep_alive<1, 2>(), py::keep_alive<1, 3>())
        .def("serialize", &CCTBuilderClass::serialize,
            py::arg("graph"),
            py::arg("file_path"),
            py::arg("serialization_format") = SerializationFormat::BOOST_BINARY,
            py::arg("compression_level") = 0)
        .def("deserialize", &CCTBuilderClass::deserialize,
            py::arg("graph"),
            py::arg("file_path"),
            py::arg("serialization_format") = SerializationFormat::BOOST_BINARY,
            py::arg("compressed") = false)
    ;
}

template<template<class> class GraphType, class... NodeDataTypes>
void bindBuilderForOneGraphTypeAndAllNodeDataTypes(py::module& m, const std::string& graphTypeName) {
    (bindBuilderForType<GraphType, NodeDataTypes>(m, graphTypeName, std::string(py::type_id<NodeDataTypes>())), ...);
}
template<template<class> class... GraphTypes>
void bindBuilderForAllGraphTypesAndAllNodeDataTypes(py::module& m) {
    std::vector<std::string> graph_types_names = {GRAPH_TYPES_AS_STRING};
    size_t index = 0;
    auto getNextElementLambda = [&]() mutable -> std::string {
        if (index < graph_types_names.size()) {
            return graph_types_names[index++];
        }
        return "";
    };
    (bindBuilderForOneGraphTypeAndAllNodeDataTypes<GraphTypes, NODE_DATA_TYPES>(m, getNextElementLambda()),...);
}

void builderBindings(py::module& m) {
    bindBuilderForAllGraphTypesAndAllNodeDataTypes<GRAPH_TYPES>(m);
}