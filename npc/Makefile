VV   = verilator
TOP  ?= top
CSRC = $(wildcard ./csrc/*.cpp)
VSRC = $(wildcard ./vsrc/*.v)
BIN  = ./obj_dir/V$(TOP)

.PHONY: all sim run clean

all: sim

sim: $(CSRC) $(VSRC)
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	$(VV) --cc --exe --build -j 0 -Wall --trace-fst \
		--top-module $(TOP) $(CSRC) $(VSRC)

run: sim
	$(BIN)

clean:
	rm -rf obj_dir

include ../Makefile