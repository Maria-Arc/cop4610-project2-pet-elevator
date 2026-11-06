/*
 * syscalls.c - kernel-side system calls (copy of <kernel>/syscalls/syscalls.c)
 *
 * Each system call forwards to a function pointer that the elevator module sets
 * when it is loaded. If the module is not loaded, the call returns -ENOSYS.
 */
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/export.h>
#include <linux/errno.h>

long (*start_elevator_ptr)(void) = NULL;
EXPORT_SYMBOL(start_elevator_ptr);

SYSCALL_DEFINE0(start_elevator)
{
	if (start_elevator_ptr)
		return start_elevator_ptr();	/* module is loaded: forward the call */
	return -ENOSYS;				/* module not loaded: nobody to answer */
}

long (*stop_elevator_ptr)(void) = NULL;
EXPORT_SYMBOL(stop_elevator_ptr);

SYSCALL_DEFINE0(stop_elevator)
{
	if (stop_elevator_ptr)
		return stop_elevator_ptr();
	return -ENOSYS;
}

long (*issue_request_ptr)(int, int, int) = NULL;
EXPORT_SYMBOL(issue_request_ptr);

SYSCALL_DEFINE3(issue_request, int, start_floor, int, destination_floor, int, type)
{
	if (issue_request_ptr)
		return issue_request_ptr(start_floor, destination_floor, type);
	return -ENOSYS;
}
