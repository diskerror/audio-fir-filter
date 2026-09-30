
# Detect Operating System
UNAME_S := $(shell uname -s)

# Language standard
STD = c++23

# Boost version
BV = 1.88

# c_lib is a sibling project; link its prebuilt, optimized libraries from
# ../c_lib/build/lib (built and tested by c_lib itself). `make` runs
# c_lib's build_libs.sh first (incremental, never cleans).
CLIB = ../c_lib
CLIB_LIBDIR = $(CLIB)/build/lib
CLIB_LIBS = $(CLIB_LIBDIR)/libdiskerror_audio.a $(CLIB_LIBDIR)/libdiskerror_program_options.a

ifeq ($(UNAME_S),Darwin)
	# Apple clang (/usr/bin), same compiler + libc++ as c_lib's prebuilt archives.
	# MacPorts clang first on PATH fails <boost/cstdfloat.hpp> (no float64_t).
	CXX = /usr/bin/clang++ -std=$(STD) -Wall -Wextra -Winvalid-pch \
		-Wno-macro-redefined -Wno-multichar -O3

    CXXFLAGS = -I/opt/local/libexec/boost/$(BV)/include \
    	-I$(CLIB) \
    	-L/opt/local/libexec/boost/$(BV)/lib

    LDLIBS = -lboost_program_options-mt
else
	# Debian 13 / Linux Configuration
	CXX = g++ -std=$(STD) -Wall -Wextra -Winvalid-pch

	CXXFLAGS = -I/usr/include \
		-I$(CLIB) \
		-L/usr/lib

	LDLIBS = -lboost_program_options
endif


SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all test clean c_lib

all: lowcut

# Always ask c_lib to bring its libraries up to date (no-op when current);
# lowcut relinks only if an archive actually changed.
c_lib:
	@$(CLIB)/build_libs.sh >/dev/null

$(CLIB_LIBS): c_lib ;

lowcut: $(SRCS) $(HDRS) $(CLIB_LIBS) Makefile
	$(CXX) $(CXXFLAGS) $(SRCS) $(CLIB_LIBS) -o $@ $(LDLIBS)

test: lowcut
	@rm -rf ~/Desktop/test\ audio
	time ./lowcut -v -f 440 -s 80 -n ~/ownCloud/test\ audio/*.{wav,aif} ~/Desktop/test\ audio
	@echo "old timing (-O3): real 0m24.040s"
	@echo "old timing (Parallel): real 0m3.307s"

clean:
	@rm -f lowcut
	@rm -rf ~/Desktop/test\ audio
