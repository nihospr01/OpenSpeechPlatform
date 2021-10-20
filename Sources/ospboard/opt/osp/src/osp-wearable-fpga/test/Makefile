# Based on http://www.armadeus.org/wiki/index.php?title=How_to_make_a_VHDL_design_in_Ubuntu/Debian
# Design files
DSNFILES = src/djb_core.vhd \
           src/car_core.vhd \
           src/djb_clock_sync.vhd \
           src/car_clock_gen.vhd \
           src/djb_sequencer.vhd \
           src/car_sequencer.vhd \
           src/sr8_rising.vhd \
           src/sr8_falling.vhd \
           src/r8_rising.vhd \
           src/r8_falling.vhd \
           src/i2s_dio.vhd
# Testbench
SIMFILES = testbench/all_tb.vhd
SIMTOP = all_tb
# Simulation break condition
GHDL_SIM_OPT = --stop-time=40us

# Setup
SIMDIR = sim
GHDL_FLAGS = -v --std=08 --ieee=standard --workdir=$(SIMDIR) --work=work

all: compile run view

compile:
	mkdir -p $(SIMDIR)
	ghdl -i $(GHDL_FLAGS) $(DSNFILES) $(SIMFILES) 
	ghdl -m $(GHDL_FLAGS) $(SIMTOP)

run:
	ghdl -r $(GHDL_FLAGS) $(SIMTOP) $(GHDL_SIM_OPT) --vcdgz=$(SIMDIR)/$(SIMTOP).vcdgz

view:
	gunzip --stdout $(SIMDIR)/$(SIMTOP).vcdgz | gtkwave --vcd

clean:
	ghdl --clean --workdir=$(SIMDIR)


