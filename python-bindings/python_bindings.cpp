#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

void serializationBindings(py::module&);
void cctBindings(py::module&);
void callGraphBindings(py::module&);
void builderBindings(py::module&);
void parserBindings(py::module&);
void perfFoldedBindings(py::module&);
void perunPinBindings(py::module&);
void perunSystemTapBindings(py::module&);

PYBIND11_MODULE(tracelibpy, m) {
    m.doc() = "pybind11 module for tracelib";

    parserBindings(m);
    perfFoldedBindings(m);
    perunPinBindings(m);
    perunSystemTapBindings(m);
    serializationBindings(m);
    cctBindings(m);
    callGraphBindings(m);
    builderBindings(m);
}
