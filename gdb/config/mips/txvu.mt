# Target: Little-endian SIM monitor board.
TDEPFILES= txvu-tdep.o mips-tdep.o
TM_FILE= tm-txvu.h
SIM_OBS = remote-sim.o
SIM = ../sim/mips/libsim.a
