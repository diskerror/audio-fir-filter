
#	Compiler
CP = clang++ -std=c++23

#	Boost version
BV = 1.81

CX = $(CP) -Wall -Winvalid-pch \
	-I/opt/local/libexec/boost/$(BV)/include \
	-L/usr/local/lib \
	-L/opt/local/libexec/boost/$(BV)/lib \
	-O3 -o $@
#	-Wl,-rpath,/usr/local/lib,/opt/local/libexec/boost/$(BV)/lib \

.PHONY: all clean

all: lowcut

lowcut: main.cp WindowedSinc.h Wave.h Makefile
	$(CX) main.cp
#	cp -f ~/ownCloud/Recordings/Treat\ Frommer\ 1987-04-26.wav ~/Desktop
#	./$@ ~/Desktop/Treat\ Frommer\ 1987-04-26.wav


clean:
	rm -f lowcut
