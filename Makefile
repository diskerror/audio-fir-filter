
# Detect Operating System
UNAME_S := $(shell uname -s)

# Language standard
STD = c++23

# Boost version
BV = 1.88


ifeq ($(UNAME_S),Darwin)
	CXX = clang++ -std=$(STD) -Wall -Wextra -Winvalid-pch \
		-Wno-macro-redefined -Wno-multichar -O3

    CXXFLAGS = -I/opt/local/libexec/boost/$(BV)/include \
    	-I../c_lib \
    	-L/opt/local/libexec/boost/$(BV)/lib \
    	-L../c_lib/lib

    LDLIBS = -lboost_program_options-mt -ldiskerror_options -ldiskerror_audio
else
	# Debian 13 / Linux Configuration
	CXX = g++ -std=$(STD) -Wall -Wextra -Winvalid-pch

	CXXFLAGS = -I/usr/include \
		-I../c_lib \
		-L/usr/lib \
		-L../c_lib/lib

	LDLIBS = -lboost_program_options -ldiskerror_options -ldiskerror_audio
endif


SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all test clean

all: lowcut

lowcut: $(SRCS) $(HDRS) makefile
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LDLIBS)

test: lowcut
	@rm -rf ~/Desktop/test\ audio
	time ./lowcut -v -f 440 -s 80 -n ~/ownCloud/test\ audio/*.{wav,aif} ~/Desktop/test\ audio
	@echo "old timing (-O3): real 0m24.040s"
	@echo "old timing (Parallel): real 0m3.307s"

clean:
	@rm -f lowcut
	@rm -rf ~/Desktop/test\ audio
