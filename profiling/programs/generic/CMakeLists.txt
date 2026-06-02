cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(zstd REQUIRED libzstd)
include_directories(${ZSTD_INCLUDE_DIR})

find_package(Boost REQUIRED NO_MODULE COMPONENTS serialization)
find_package(nlohmann_json REQUIRED)
find_package(absl CONFIG REQUIRED)


# Add target
add_executable(generic)
target_compile_options(generic PRIVATE -g -Wno-unused-parameter -flto=auto)
target_sources(generic PRIVATE
        main.cpp
)
target_link_libraries(generic PUBLIC tracelib PRIVATE
        absl::flat_hash_map
        Boost::serialization
        nlohmann_json::nlohmann_json
        zstd
        ${ZSTD_LIBRARY}
)
