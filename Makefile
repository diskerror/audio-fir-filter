
#	Compiler
CP = clang++ -std=c++23

#	Boost version
BV = 1.87

CX = $(CP) -Wall -Winvalid-pch -Wno-macro-redefined -O3 -I/opt/local/include/ -I/opt/local/libexec/boost/$(BV)/include

LIBS= -L/usr/local/lib -L/opt/local/lib -L/opt/local/libexec/boost/$(BV)/lib -lboost_program_options-mt

SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all test clean

all: lowcut

lowcut: $(SRCS) $(HDRS) makefile
	$(CX) $(SRCS) -o $@
#	rm -rf ~/Desktop/test\ audio
#	cp -fr ~/ownCloud/test\ audio ~/Desktop
#	./$@ ~/Desktop/test\ audio/*{.wav,.aif}

test:
	rm -rf ~/Desktop/test\ audio
	cp -a ~/ownCloud/test\ audio ~/Desktop
	./lowcut -f 330 -s 80 -nv ~/Desktop/test\ audio/*{.wav,.aif}

clean:
	rm lowcut
	rm -rf ~/Desktop/test\ audio
