all:
	gcc -o iopattern iopattern.c

clean:
	rm -f iopattern

check:
	cstyle iopattern.c
