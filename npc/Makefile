all: sim

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@verilator -Wall --cc --exe --build --trace sim_main.cpp top.v
	@./obj_dir/Vtop

wave:
	@gtkwave waveform.vcd &

clean:
	@rm -rf obj_dir
	@rm -f waveform.vcd

include ../Makefile

