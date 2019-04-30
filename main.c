#define _GNU_SOURCE             
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#define PROCESS_NUM 1000
struct timeval tv;

unsigned long st_time[100000][2], en_time[100000][2];
static void my_sig(int);
void init_signal() {
	struct sigaction act;
	act.sa_handler = my_sig;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		fprintf(stderr, "sigaction err\n");
		exit(1);
	}
	if (sigaction(SIGUSR2, &act, NULL) == -1) {
		fprintf(stderr, "sigaction err\n");
		exit(1);
	}
}
char policy[20];
int N;
int cur = 0;
int pri[PROCESS_NUM] = {};
int fin = 0;
typedef struct ts {
	pid_t pid;
	char name[36];
	int ready_time, exec_time;
	int real_ready_time, real_finish_time;
}Task;
Task task[PROCESS_NUM]; 
void change(int flag) {
	int now = -1;
	for (int i = 0; i < PROCESS_NUM; i++) {
		if (pri[i] == 99) {
			now = i;
			break;
		}
	}
	if (now == -1) {
		fprintf(stderr, "change error\n");
		exit(1);
	}
	int nxt = now;
	for (int i = now+1; i != now; i = (i+1) % PROCESS_NUM) {
		if (pri[i] == 98) {
			nxt = i;
			break;
		}
	}
	if (flag == 1) {
		pri[nxt] = 99;
		pri[now] = 0; 
	}
	if (flag == 2) {
		pri[now] = 98;
		pri[nxt] = 99;
	}
	for (int i = 0; i < PROCESS_NUM; i++) {
		if (pri[i] != 0) {
			struct sched_param param;
			param.sched_priority = pri[i];
			if( sched_setscheduler(task[i].pid, SCHED_FIFO, &param) <0 ) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
		}
	}
}

int record[PROCESS_NUM] = {}, now_psjf = -1;
int start_time = 0;
int reorder() {
	int order_flag = 0;
	int mintime = 1000000000;
	int store = -1;
	for(int i = 0; i < N; i++){
		if(record[i]){
			order_flag = 1;
			if(task[i].exec_time < mintime){
				mintime = task[i].exec_time;
				store = i;
			}
		}
	}
	return store;
}


static void my_sig(int signo) {
	if (signo == SIGUSR1) {
		pid_t child = wait(NULL);
		syscall(333, &en_time[child][0], &en_time[child][1]);
		syscall(334, child, st_time[child][0], st_time[child][1], en_time[child][0], en_time[child][1]);
		cur --;
		if (policy[0] == 'R') {
			change(1);
		}
		if (policy[0] == 'P') {
			record[now_psjf] = 0;
			now_psjf = reorder();
			if(now_psjf != -1){
				struct sched_param param;
				param.sched_priority = sched_get_priority_max(SCHED_FIFO);
				if( sched_setscheduler(task[now_psjf].pid, SCHED_FIFO, &param) <0 ) {
						fprintf(stderr, "%s\n", strerror(errno));
						exit(1);
				}
				start_time += task[now_psjf].exec_time;
			}
		}
		fin++;
	}
	if (signo == SIGUSR2) {
		if (strcmp("RR", policy) == 0) {
			change(2);
		}
	}

}

void set_CPU(pid_t pid) {
	cpu_set_t cmask;
    unsigned long len = sizeof(cmask);
    CPU_ZERO(&cmask);   
    CPU_SET(0, &cmask);
	if (sched_setaffinity(pid, len, &cmask) < 0) {
		fprintf(stderr, "close error with msg is: %s\n",strerror(errno));
   		exit(1);
 	}
}

void timer(int units) { 
	volatile unsigned long i;
	for (int t = 0; t < units; t++) {
		for ( i = 0; i < 1000000UL; i++ ) {
			;
		}
	}
}
int fd[2];
pid_t create(int units) {
	pid_t pid;
	char buf[10];
	pipe(fd);
	if ( (pid = fork()) > 0) {
		return pid;
	} else if (pid == 0){
		char inp[10] = {};
		sprintf(inp, "%d", units);
		read(fd[0], buf, 1);
		set_CPU(0);
		if (execl("./task", "task", inp, NULL) < 0) {
			fprintf(stderr, "exec error\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "create error\n");
		exit(1);
	}
	return 0;
}


void FIFO() {
	int progress = 0;
	for (int t = 0; ; t++) {
		while (task[progress].ready_time == t) {
			task[progress].pid = create(task[progress].exec_time);
			struct sched_param param;
			param.sched_priority = sched_get_priority_max(SCHED_FIFO);
			if( sched_setscheduler(task[progress].pid, SCHED_FIFO, &param) <0 ) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
			write(fd[1], "1", 1);
			
			syscall(333, &st_time[task[progress].pid][0], &st_time[task[progress].pid][1]);

			progress++;
			if (progress == N) {
				return;
			}
		}
		timer(1);
	}
}

void RR() {
	int progress = 0;
	for (int t = 0; ; t++) {
		while (task[progress].ready_time == t) {
			task[progress].pid = create(task[progress].exec_time);
			struct sched_param param;
			param.sched_priority = sched_get_priority_max(SCHED_FIFO);
			if (cur > 0) {
				param.sched_priority -= 1;
			}
			pri[progress] = param.sched_priority;
			if( sched_setscheduler(task[progress].pid, SCHED_FIFO, &param) <0 ) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
			write(fd[1], "1", 1);
			
			syscall(333, &st_time[task[progress].pid][0], &st_time[task[progress].pid][1]);

			progress++;
			cur++;
			if (progress == N) {
				return;
			}
		}
		timer(1);
	}
}

void SJF() {
	int progress = 0;
	int used[PROCESS_NUM] = {};
	int seq[PROCESS_NUM] = {};
	int now = 0;
	int cnt = 0;
	for (int i = 0; i < N; i++) {
		int mintime = 1000000000;
		for (int j = 0; j < N; j++) {
			if (!used[j] && mintime > task[j].ready_time) {
				mintime = task[j].ready_time;
			}
		}
		if (now < mintime) {
			now = mintime;
		}
		int min = 1000000000;
		int choose = -1;
		for (int j = 0; j < N; j++) {
			if (task[j].ready_time <= now && !used[j]) {
				if (min > task[j].exec_time) {
					min = task[j].exec_time;
					choose = j;
				}
			}
		}
		used[choose] = 1;
		printf("%d\n", choose+1);
		seq[cnt++] = choose;
		now += task[choose].exec_time;
	}


	cnt = 0;
	for (int t = 0; ; t++) {
		while (task[progress].ready_time == t) {
			task[progress].pid = create(task[progress].exec_time);
			struct sched_param param;
			int s;
			for (s = 0; s < N; s++) {
				if (seq[s] == progress) {
					break;
				}
			}
			param.sched_priority = sched_get_priority_max(SCHED_FIFO)-s;
			if( sched_setscheduler(task[progress].pid, SCHED_FIFO, &param) <0 ) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
			write(fd[1], "1", 1);
			
			syscall(333, &st_time[task[progress].pid][0], &st_time[task[progress].pid][1]);

			progress++;
			if (progress == N) {
				return;
			}
		}
		timer(1);
	}
}
void PSJF() {
	int progress = 0;
	now_psjf = -1;
	for (int t = 0; ; t++) {
		while (task[progress].ready_time == t) {
			record[progress] = 1;
			task[progress].pid = create(task[progress].exec_time);
			if(now_psjf == -1) {
				now_psjf = progress;
				start_time = t;
			}
			else{
				int before = now_psjf;
				struct sched_param param;
				param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
				if( sched_setscheduler(task[now_psjf].pid, SCHED_FIFO, &param) <0 ) {
						fprintf(stderr, "%s\n", strerror(errno));
						exit(1);
					}
				
				task[now_psjf].exec_time -= t - start_time;
				now_psjf = reorder();
				if(before != now_psjf) start_time = t;
			}
			struct sched_param param;
			param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
			if( sched_setscheduler(task[progress].pid, SCHED_FIFO, &param) <0 ) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
			param.sched_priority = sched_get_priority_max(SCHED_FIFO);
			if( sched_setscheduler(task[now_psjf].pid, SCHED_FIFO, &param) <0 ) {
					fprintf(stderr, "%s\n", strerror(errno));
					exit(1);
				}
			write(fd[1], "1", 1);
			
			syscall(333, &st_time[task[progress].pid][0], &st_time[task[progress].pid][1]);

			progress++;
			if (progress == N) {
				return;
			}
		}
		timer(1);
	}
}
void init() {
	init_signal();
	scanf("%s%d", policy, &N);
	return;
}

void solve() {
	for (int i = 0; i < N; i++) {
		scanf("%s%d%d", task[i].name, &task[i].ready_time, &task[i].exec_time); 
	}
	if (strcmp(policy, "FIFO") == 0 ) {
		FIFO();
	}
	if (strcmp(policy, "RR") == 0) {
		RR();
	}
	if (strcmp(policy, "SJF") == 0) {
		SJF();
	}
	if (strcmp(policy, "PSJF") == 0) {
		PSJF();
	}

	return;
}


int main() {
	init();
	solve();
	while(fin != N);
}
