#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/proc_fs.h> 
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("A simple Linux kernel module to keep track of current time and time since last call"); 
MODULE_VERSION("1.0");

#define ENTRY_NAME "timer" //for proc/timer
#define PERMS 0444         //only r permission
#define PARENT NULL
#define BUF_LEN 100

static struct proc_dir_entry* proc_entry; 
static int procfs_buf_len;
static char msg[BUF_LEN];

static struct timespec64 prev_time;
static bool has_prev;


static ssize_t timer_read(struct file* file, char* ubuf, size_t count, loff_t
*ppos) {

    struct timespec64 current_time, sub_time;
    int len;

        
    if (*ppos > 0 )
        return 0;

    ktime_get_real_ts64(&current_time);
    len = snprintf(msg, BUF_LEN, "current time: %lld.%09ld\n",
                   (long long)current_time.tv_sec, current_time.tv_nsec);
    

    if (has_prev)

    {
        sub_time = timespec64_sub(current_time, prev_time);
        len += snprintf(msg + len, BUF_LEN - len, "elapsed time: %lld.%09ld\n",
                        (long long)sub_time.tv_sec, sub_time.tv_nsec);
    }

    
    procfs_buf_len = len; 

   if (count < procfs_buf_len)
        return 0;
    if (copy_to_user(ubuf, msg,  procfs_buf_len)) 
        return -EFAULT;

    prev_time = current_time;
    has_prev = true;
    
    *ppos =  procfs_buf_len;
    
    return  procfs_buf_len;
}



static const struct proc_ops procfile_fops = {
.proc_read = timer_read
};

static int __init timer_init(void) { 
    proc_entry = proc_create(ENTRY_NAME, PERMS,PARENT, &procfile_fops);
    if (proc_entry == NULL)
        return -ENOMEM; 
    return 0;
}

static void __exit timer_exit(void) {
     proc_remove(proc_entry);
}


module_init(timer_init); 
module_exit(timer_exit);
