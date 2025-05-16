#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tracelib/perunPin.hpp"
#include "tracelib/parser.hpp"

namespace py = pybind11;

void perunPinBindings(py::module& m) {

    py::class_<PerunPinParser::Location>(m, "PerunPinLocationMetadata")
        .def_readonly("file_id", &PerunPinParser::Location::fileId)
        .def_readonly("lines", &PerunPinParser::Location::lines)
    ;

    py::class_<PerunPinParser::BasicBlockMetadata>(m, "PerunPinBasicBlockMetadata")
        .def_readonly("function_name", &PerunPinParser::BasicBlockMetadata::functionName)
        .def_readonly("instructions_cnt", &PerunPinParser::BasicBlockMetadata::instructionsCnt)
        .def_readonly("location", &PerunPinParser::BasicBlockMetadata::location)
    ;

    py::class_<PerunPinParser::FunctionMetadata>(m, "PerunPinFunctionMetadata")
        .def_readonly("name", &PerunPinParser::FunctionMetadata::name)
        .def_readonly("argument_indices", &PerunPinParser::FunctionMetadata::argumentIndices)
        .def_readonly("location", &PerunPinParser::FunctionMetadata::location)
    ;

    py::class_<PerunPinParser::Metadata>(m, "PerunPinMetadata")
        .def_readonly("file_paths", &PerunPinParser::Metadata::filePaths)
        .def_readonly("functions", &PerunPinParser::Metadata::functions)
        .def_readonly("basi_blocks", &PerunPinParser::Metadata::basicBlocks)
    ;

    py::class_<PerunPinParser, Parser>(m, "PerunPinParser")
        .def(py::init<const std::string&, const std::string&>(),
            py::arg("traceFilePath"), py::arg("metadataFIlePath") = "")
        .def_readonly("metadata", &PerunPinParser::metadata)
    ;

    py::class_<PerunPinNodeData>(m, "PerunPinNodeData")
        .def(py::init<>())
        .def_readonly("durations", &PerunPinNodeData::durations)
        .def_readonly("file_path", &PerunPinNodeData::filePath)
        .def_readonly("lines", &PerunPinNodeData::lines)
        .def_readonly("argument_values", &PerunPinNodeData::argumentValues)
        .def_readonly("basic_block_durations_map", &PerunPinNodeData::basicBlockDurationsMap)
    ;
}
