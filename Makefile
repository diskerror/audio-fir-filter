
#	Compiler
COMP = clang++ -std=c++23 -Wall -Winvalid-pch -Wno-macro-redefined -O3 \
    -I/opt/local/include/ -I/opt/local/libexec/boost/1.87/include

SRCS=$(wildcard *.cp)
HDRS=$(wildcard *.h)

.PHONY: all test clean

all: lowcut

lowcut: $(SRCS) $(HDRS) makefile
	$(COMP) $(SRCS) -o $@

test: lowcut
	@rm -rf ~/Desktop/test\ audio
	@cp -a ~/ownCloud/test\ audio ~/Desktop
	time ./lowcut -f 330 -s 80 -n ~/Desktop/test\ audio/*.{wav,aif}

clean:
	rm lowcut
	rm -rf ~/Desktop/test\ audio
