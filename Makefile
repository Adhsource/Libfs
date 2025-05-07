mklibfs: mklibfs.c
	gcc mklibfs.c -o mklibfs


test: test.c struct.h fonction.c
	gcc test.c fonction.c -o test

#test: test.c struct.h
#	gcc test.c -o test

clean:
	rm -f libfs.fs
	rm -f mklibfs
	rm -f test
	rm -f core*
