all: ftserver.c
	gcc -o ftserver ftserver.c

clean: 
	rm -f ftserver
