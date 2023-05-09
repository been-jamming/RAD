CC = cc
DEL = rm -r
DIR = mkdir -p
FLAGS = -lm -Wall -pedantic -g
LINKDIR = -L.

neuron_test: librad.a neurons.c
	$(CC) $(LINKDIR) neurons.c -lrad -lm $(FLAGS) -o neuron_test

librad.a: rad.o parse.o
	ar -rc librad.a rad.o parse.o

rad.o: rad.c
	$(CC) rad.c $(FLAGS) -c -o rad.o

parse.o: parse.c
	$(CC) parse.c $(FLAGS) -c -o parse.o

clean:
	$(DEL) neuron_test ||:
	$(DEL) librad.a ||:
	$(DEL) rad.o ||:
	$(DEL) parse.o ||:
