all: sic_xe.o
	gcc -Wall sic_xe.c -o sic_xe.out
clean:
	rm sic_xe.out *.o
