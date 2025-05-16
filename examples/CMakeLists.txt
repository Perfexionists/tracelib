
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED NO_MODULE COMPONENTS serialization)
find_package(zstd REQUIRED)
find_package(nlohmann_json REQUIRED)


# Add example target
add_executable(example)
target_compile_options(example PRIVATE -g -Wno-unused-parameter -flto=auto)
target_sources(example PRIVATE
        program/example.cpp
)
target_link_libraries(example PUBLIC tracelib PRIVATE
        Boost::serialization
        nlohmann_json::nlohmann_json
        zstd
)

# Add python example to the build
configure_file(python-program/example.py ${CMAKE_CURRENT_BINARY_DIR}/example.py COPYONLY)

# Add the example inputs to the folder as well
file(COPY ./input DESTINATION ${CMAKE_CURRENT_BINARY_DIR})