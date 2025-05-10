mklibfs: mklibfs.c
	gcc mklibfs.c -o mklibfs


test: test.c struct.h fonction.c
	gcc test.c fonction.c -o test -g3


clean:
	rm -f libfs.fs
	rm -f mklibfs
	rm -f test
	rm -f core*
core:
	rm -f core*
clear:
	clear

file:	mklibfs
	./mklibfs -s 200 -i 32

quick: clear core test
	./test

