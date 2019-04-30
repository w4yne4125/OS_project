#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
void timer(int units) {
	volatile unsigned long i;
	for (int t = 1; t <= units; t++) {
		for ( i = 0; i < 1000000UL; i++) {
			;
		}
		if (t % 500 == 0 && t != units) {
			kill(getppid(), SIGUSR2);
		}
	}
}

int main(int argc, char** argv){
	int n;
	sscanf(argv[1], "%d", &n);
	timer(n);
	kill(getppid(), SIGUSR1);
	return 0;
}

