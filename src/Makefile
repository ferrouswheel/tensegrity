PROGRAMS = spinhard

# Important optimization options
CFLAGS = -O3 -ffast-math -fno-rtti

# Standard libraries
LFLAGS = -lm -lstdc++ -lpthread

# Debugging
CFLAGS += -g -Wall
LFLAGS += -g

# Annoying warnings on by default on Mac OS
CFLAGS += -Wno-tautological-constant-out-of-range-compare -Wno-gnu-static-float-init


all: $(PROGRAMS)

.c:
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

.cpp:
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

.o:
	$(CC) $(CFLAGS) $< -o $@

spinhard: spinhard.o rotaryencoder.o
	$(CC) $(CFLAGS) spinhard.o rotaryencoder.o -o $@ $(LFLAGS)

.PHONY: clean all

clean:
	rm -f $(PROGRAMS)
