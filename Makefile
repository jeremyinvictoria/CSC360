main: main.o
	gcc main.o -o main
main.o: main.c
	gcc -c main.c
.PHONY clean:
clean:
	rm -f main main.o
