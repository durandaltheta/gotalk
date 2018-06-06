cc = gcc
source = "src"

all: gotalk

libmill: 
	cd externals/libmill 
	./configure 
	make 
	make check 
	sudo make install
	cd ../../

gotalk: libmill $(source)/gotalk.c $(source)/gotalk.h
	mkdir -p bld
	$(cc) -c gotalk.c -o gotalk.o

install: gotalk 
	sudo cp gotalk.o /usr/local/bin/ 
	sudo cp gotalk.h /usr/local/include/

.PHONY : clean
clean :
	rm -rf bld
