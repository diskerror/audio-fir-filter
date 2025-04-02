
#	Compiler
CP = clang++ -std=c++23

#	Boost version
BV = 1.81

CX = $(CP) -Wall -Winvalid-pch -Wno-macro-redefined -O3 -I/opt/local/libexec/boost/$(BV)/include

LIBS= -L/usr/local/lib -L/opt/local/libexec/boost/$(BV)/lib

SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all

all: lowcut

lowcut: $(SRCS) $(HDRS) makefile
	$(CX) $(SRCS) -o $@
	cp -fr ~/ownCloud/test\ audio ~/Desktop
	./$@ ~/Desktop/test\ audio/*.wav
