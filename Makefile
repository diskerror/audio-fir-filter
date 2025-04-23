
#	Compiler
CP = clang++ -std=c++23

#	Boost version
BV = 1.87

CX = $(CP) -Wall -Winvalid-pch -Wno-macro-redefined -O3 -I/opt/local/include/ -I/opt/local/libexec/boost/$(BV)/include

LIBS= -L/usr/local/lib -L/opt/local/lib -L/opt/local/libexec/boost/$(BV)/lib -lboost_program_options-mt

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
	cp -r ~/ownCloud/test\ audio ~/Desktop
	./lowcut -n ~/Desktop/test\ audio/*{.wav,.aif}
