CPP = g++
CPP_FLAGS = -Wall -O3 -L/usr/local/lib
MODULE = cjk-tokenizer

all: build

build: cjk-hanconvert.o cjk-tokenizer.o libcjk-tokenizer.a

cjk-hanconvert.o:
	$(CPP) $(CPP_FLAGS) -c cjk-hanconvert.cc

cjk-tokenizer.o:
	$(CPP) $(CPP_FLAGS) -c cjk-tokenizer.cc

libcjk-tokenizer.a: cjk-tokenizer.o
	ar crv $@ cjk-tokenizer.o cjk-hanconvert.o
	ranlib $@

test:
	$(CPP) $(CPP_FLAGS) -static -o $@ $@.cc -L. -lcjk-tokenizer -lunicode
	./$@

clean:
	rm -rf *.o *~ *.a test
