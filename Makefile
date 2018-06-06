cc = gcc
source = src
build = bld
go = "externals/libmill/"

all: gotalk

libmill: 
	cmake $(go) \
	make $(go) \
	sudo make install $(go)

gotalk: libmill $(source)/gotalk.c $(source)/gotalk.h
	mkdir -p $(build); \
	$(cc) -c $(source)/gotalk.c -o $(build)/gotalk.o

install: gotalk 
	sudo cp gotalk.o /usr/local/bin/ \
	sudo cp gotalk.h /usr/local/include/

.PHONY : clean
clean :
	rm -rf bld
