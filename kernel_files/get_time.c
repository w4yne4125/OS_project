#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/time.h>

asmlinkage int sys_get_time(unsigned long *sec, unsigned long *nsec){
	struct timespec now;
	getnstimeofday(&now);
	*sec = now.tv_sec;
	*nsec = now.tv_nsec;
	return 1;
}
