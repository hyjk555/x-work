WARNINGS=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -Waggregate-return \
	 -Wcast-align -Wcast-qual -Wnested-externs -Wshadow -Wbad-function-cast \
	 -Wwrite-strings

CFLAGS+=-ffast-math -fomit-frame-pointer -static -pthread

ifeq "$(DATATYPE)" ""
	DATATYPE=double
endif

ifeq  "$(DATATYPE)" "int16_t"
	TYPEFLAGS=-DFIXED_POINT=16
endif

ifeq  "$(DATATYPE)" "int32_t"
	TYPEFLAGS=-DFIXED_POINT=32
endif

TYPEFLAGS=-Dkiss_fft_scalar=$(DATATYPE)

SRCFILES=dmic-test.c wav.c

all: clean dmic-test

dmic-test: $(SRCFILES)
	mips-linux-gnu-gcc -o $@ $(CFLAGS) $(TYPEFLAGS) -DREAL_FASTFIR -lm -lpthread $+ -DFAST_FILT_UTIL

clean:
	rm -rf dmic-test
