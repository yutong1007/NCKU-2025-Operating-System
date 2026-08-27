#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <asm/current.h>

#define procfs_name "Mythread_info"
#define BUFSIZE  1024
char buf[BUFSIZE]; //kernel buffer
static char stored_msg[BUFSIZE]; 
static DEFINE_MUTEX(msg_lock);

static ssize_t Mywrite(struct file *fileptr, const char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    // user write length
    int len = buffer_len;
    if (len >= BUFSIZE)
    	len = BUFSIZE - 1;

    // copy to kernel buffer
    if (copy_from_user(buf, ubuf, len))
    	return -EFAULT;

    buf[len] = '\0';

    // remove \n
    if (len > 0 && buf[len - 1] == '\n')
    	buf[len - 1] = '\0';

    return buffer_len;
    /****************/
}


static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    int len;
    unsigned long long time_ms;

    if (*offset > 0)
    	return 0;

    // caculate time = utime / 100 / 1000
    time_ms = (unsigned long long)current->utime / 100 / 1000;

    // output current thread info
    len = scnprintf(buf, BUFSIZE,
                "%s\nPID: %d, TID: %d, time: %llu\n",
                buf,
                current->tgid,
                current->pid,
                time_ms);

    if (len > buffer_len)
    	len = buffer_len;

    // copy to user
    if (copy_to_user(ubuf, buf, len))
    	return -EFAULT;

    // update offset
    *offset += len;
    return len;
    /****************/
}

static struct proc_ops Myops = {
    .proc_read = Myread,
    .proc_write = Mywrite,
};

static int My_Kernel_Init(void){
    proc_create(procfs_name, 0666, NULL, &Myops);   
    pr_info("My kernel says Hi");
    return 0;
}

static void My_Kernel_Exit(void){
    pr_info("My kernel says GOODBYE");
}

module_init(My_Kernel_Init);
module_exit(My_Kernel_Exit);

MODULE_LICENSE("GPL");
