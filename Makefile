
#	Compiler
CP = clang++ -std=c++23

#	Boost version
BV = 1.87

CX = $(CP) -Wall -Winvalid-pch -Wno-macro-redefined -O3 -I/opt/local/libexec/boost/$(BV)/include -I/usr/local/include

LIBS= -L/usr/local/lib -L/opt/local/lib -L/opt/local/libexec/boost/$(BV)/lib -lboost_program_options

SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all

all: lowcut

lowcut: $(SRCS) $(HDRS) makefile
	$(CX) $(SRCS) -o $@
#	rm -rf ~/Desktop/test\ audio
#	cp -fr ~/ownCloud/test\ audio ~/Desktop
#	./$@ ~/Desktop/test\ audio/*{.wav,.aif}

test:
	rm -rf ~/Desktop/test\ audio
	cp -fr ~/ownCloud/test\ audio ~/Desktop
	./lowcut ~/Desktop/test\ audio/*{.wav,.aif}
