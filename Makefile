
#	Compiler
CP = clang++ -std=c++23

#	Boost version
BV = 1.81

CX = $(CP) -Wall -Winvalid-pch -O3 -I/opt/local/libexec/boost/$(BV)/include

LIBS= -L/usr/local/lib -L/opt/local/libexec/boost/$(BV)/lib

SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all clean echo

all: lowcut

lowcut: $(SRCS) $(HDRS) makefile
	$(CX) $(OBJS) $(SRCS) -o $@
#	cp -f ~/ownCloud/Recordings/Treat\ Frommer\ 1987-04-26.wav ~/Desktop
#	./$@ ~/Desktop/Treat\ Frommer\ 1987-04-26.wav


clean:
	@rm -f lowcut ${OBJS}

echo:
	@echo ${SRCS}
	