#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("A simple Linux kernel module to check systemcalls.");
MODULE_VERSION("0.1");

extern long (*start_elevator_ptr)(void);
extern long (*issue_request_ptr)(int, int, int);
extern long (*stop_elevator_ptr)(void);

static long start_elevator(void) { return 0; }
static long issue_request(int start_floor, int destination_floor, int type) { return 0; }
static long stop_elevator(void) { return 0; }

static int __init syscheck_init(void)
{
	start_elevator_ptr = start_elevator;
	issue_request_ptr = issue_request;
	stop_elevator_ptr = stop_elevator;
	return 0;
}

static void __exit syscheck_exit(void)
{
	start_elevator_ptr = NULL;
	issue_request_ptr = NULL;
	stop_elevator_ptr = NULL;
}

module_init(syscheck_init);
module_exit(syscheck_exit);
