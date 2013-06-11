.PHONY:all clean
all: test testGCC mm2bin

CPP=g++
CC=gcc

### Additional options for the T2 Sun machine. Uncomment to 
### activate them.
# CPPOPTS=-m64 -mcpu=v9 -MMD -lrt -D __BIG_ENDIAN_XBO
#CPPOPTS=-D DEBUG
CCOPTS=-std=gnu99

xboutils.o: xboutils.c xboutils.h
	$(CPP) $(CPPOPTS) -c xboutils.c -o $@

test: test.c xboutils.o
	$(CPP) $(CPPOPTS) -o test test.c xboutils.o

xboutilsGCC.o: xboutils.c xboutils.h
	$(CC) $(CCOPTS) -c xboutils.c -o $@

testGCC: test.c xboutilsGCC.o
	$(CC) $(CCOPTS) -o testGCC test.c xboutilsGCC.o

mm2bin: convertMMtoBin.c readMMformat.c
	$(CC) $(CCOPTS) convertMMtoBin.c -o $@

clean:
	rm -rf xboutils.o test xboutilsGCC.o testGCC.o mm2bin
