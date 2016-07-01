MBED = mbed
TOOLCHAIN = GCC_ARM
MCU = K64F

MFLAGS += -j 4
ifdef VERBOSE
MFLAGS += -v
endif
ifdef DEBUG
MFLAGS += -o debug-info
endif

all:
	$(MBED) compile $(MFLAGS) -t $(TOOLCHAIN) -m $(MCU) $(addprefix --source=, . $(SRC))

clean:
	rm -rf .build
