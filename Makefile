
#	Compiler
CP 	= clang++

#	Boost version
BV = 1.87

CXXFLAGS = -std=c++23 -Wall -Winvalid-pch -Wno-macro-redefined  -O3 \
	-I/opt/local/include/ -I/opt/local/libexec/boost/$(BV)/include -I../c_lib \
	-L/usr/local/lib -L/opt/local/libexec/boost/$(BV)/lib

SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all test clean

all: lowcut

lowcut: $(SRCS) $(HDRS) makefile
	$(CP) $(CXXFLAGS) $(SRCS) -o $@

test: lowcut
	@rm -rf ~/Desktop/test\ audio
	@cp -a ~/ownCloud/test\ audio ~/Desktop
	time ./lowcut -f 330 -s 80 -n ~/Desktop/test\ audio/*.{wav,aif}

clean:
	rm lowcut
	rm -rf ~/Desktop/test\ audio
