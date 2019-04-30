all: main task 
	gcc main.o -o main 
	gcc task.o -o task
main.o: main.c 
	gcc -c main.c
task.o: task.c 
	gcc -c task.c

