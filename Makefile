# Makefile for image retrieval program.

FLAGS= -Wall -std=gnu99 -lm

all : one_process

one_process : one_process.o worker.o image_retrieval.o
	gcc -o $@ one_process.o worker.o image_retrieval.o ${FLAGS}


# Separately compile each C file
%.o : %.c
	gcc -c $< ${FLAGS}

one_process.o : worker.h
worker.o : worker.h
image_retrieval.o : worker.h

clean :
	-rm *.o one_process
