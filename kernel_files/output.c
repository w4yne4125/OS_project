#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage int sys_output(pid_t pid, unsigned long st_sec, unsigned long st_nsec, unsigned long en_sec, unsigned long en_nsec){
	printk("[Project1] %d %lu.%lu %lu.%lu\n", pid, st_sec, st_nsec, en_sec, en_nsec);
	return 1;
}	
