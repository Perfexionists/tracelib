cmake_minimum_required(VERSION 3.24)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
message("-- Python_SITEARCH: ${Python_SITEARCH}")
find_package(Boost REQUIRED NO_MODULE COMPONENTS serialization)
find_package(zstd REQUIRED)
find_package(nlohmann_json REQUIRED)

include(FetchContent)
FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(tracelibpy)
target_sources(tracelibpy PRIVATE
    python_bindings.cpp
    serialization_bindings.cpp
    callingContextTree_bindings.cpp
    callGraph_bindings.cpp
    builder_bindings.cpp
    parser_bindings.cpp
    perfFolded_bindings.cpp
    perunPin_bindings.cpp
    perunSystemTap_bindings.cpp
)
target_compile_options(tracelibpy PRIVATE -g -Wno-unused-parameter -flto=auto)
target_link_libraries(tracelibpy
    PRIVATE tracelib Boost::serialization zstd nlohmann_json
)

# This wont work with the cmake's variable for custom installation prefix since it is an absolute path
install(TARGETS tracelibpy
        ARCHIVE  DESTINATION ${Python_SITEARCH}
        LIBRARY  DESTINATION ${Python_SITEARCH}
)
