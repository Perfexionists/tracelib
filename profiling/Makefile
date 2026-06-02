# Uses Brendan Gregg's flame graph library ( https://github.com/brendangregg/flamegraph )
# Set this variable to the path where it is cloned on your machine.
FLAMEGRAPH_LIB := $(FLAME_GRAPH)

BINARIES := "../build/profiling/programs/"

.PHONY=build

# Big samples are gitignored. Please provide your own.
time: build
	hyperfine -w 5 -L threads 1,2,4,6,8 "${BINARIES}/generic/generic {threads} parBuild ./samples/big/big.perf-folded" # --export-csv hyperfine.out.csv

flamegraph: build
	sudo perf record -F 99 -g -o - -- "${BINARIES}/generic/generic" 4 parBuild ./samples/big/big.perf-folded \
	| sudo perf script -i - \
	| ${FLAMEGRAPH_LIB}/stackcollapse-perf.pl \
	| ${FLAMEGRAPH_LIB}/flamegraph.pl --minwidth 0.0 > perf.svg

build:
	cmake -S .. -B ../build -DINCLUDE_PROFILING_PROGRAMS=ON
	cmake --build ../build -j
	sudo cmake --install ../build
	sudo ldconfig

