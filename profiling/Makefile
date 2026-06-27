# Uses Brendan Gregg's FlameGraph library ( https://github.com/brendangregg/FlameGraph )
# The two Perl scripts the flamegraph target needs are downloaded automatically into
# FLAMEGRAPH_LIB (defaults to ./libs). Override it to reuse an existing checkout.
FLAMEGRAPH_LIB ?= libs
# Raw download location of the scripts.
FLAMEGRAPH_RAW_URL := https://raw.githubusercontent.com/brendangregg/FlameGraph/master
FLAMEGRAPH_SCRIPTS := $(FLAMEGRAPH_LIB)/stackcollapse-perf.pl $(FLAMEGRAPH_LIB)/flamegraph.pl

BINARIES := "../build/profiling/programs/"

.PHONY = build time flamegraph
# Remove a partially downloaded script if wget fails, so the next run retries cleanly.
.DELETE_ON_ERROR:

# Big samples are gitignored. Please provide your own.
time: build
	hyperfine -w 5 -L threads 1,2,4,6,8 "${BINARIES}/generic/generic {threads} parBuild ./samples/big/big.perf-folded" # --export-csv hyperfine.out.csv

flamegraph: build $(FLAMEGRAPH_SCRIPTS)
	sudo perf record -F 99 -g -o - -- "${BINARIES}/generic/generic" 4 parBuild ./samples/big/big.perf-folded \
	| sudo perf script -i - \
	| ${FLAMEGRAPH_LIB}/stackcollapse-perf.pl \
	| ${FLAMEGRAPH_LIB}/flamegraph.pl --minwidth 0.0 > perf.svg

build:
	cmake -S .. -B ../build -DINCLUDE_PROFILING_PROGRAMS=ON
	cmake --build ../build -j
	sudo cmake --install ../build
	sudo ldconfig

# Fetch a single FlameGraph Perl script on demand into $(FLAMEGRAPH_LIB).
# The rule only runs when the target file is missing, so scripts are downloaded once.
$(FLAMEGRAPH_LIB)/%.pl:
	mkdir -p $(FLAMEGRAPH_LIB)
	wget -q -O $@ $(FLAMEGRAPH_RAW_URL)/$*.pl
	chmod +x $@

