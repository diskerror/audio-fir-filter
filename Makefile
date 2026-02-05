
#	Compiler (not clang)
CP 	= g++ -std=c++23 -Wall -Wextra -Winvalid-pch -Wno-macro-redefined -O3

#	Boost version
BV = 1.88

CXXFLAGS = -I/opt/local/libexec/gcc15/libc++/include \
	-I/opt/local/libexec/boost/$(BV)/include \
	-I../c_lib \
	-L/opt/local/libexec/boost/$(BV)/lib \
	-L../c_lib/lib -ldiskerror_audio

SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all test clean

all: lowcut

lowcut: $(SRCS) $(HDRS) makefile
	$(CP) $(CXXFLAGS) $(SRCS) -o $@

test: lowcut
	@rm -rf ~/Desktop/test\ audio
	time ./lowcut -v -f 440 -s 80 -n ~/ownCloud/test\ audio/*.{wav,aif} ~/Desktop/test\ audio
	@echo "old timing (-O3): real 0m24.040s"
	@echo "old timing (Parallel): real 0m3.307s"

clean:
	@rm -f lowcut
	@rm -rf ~/Desktop/test\ audio
