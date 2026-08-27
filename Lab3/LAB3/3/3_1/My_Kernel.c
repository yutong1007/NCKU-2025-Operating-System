#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <asm/current.h>

#define procfs_name "mythread_info"
#define BUFSIZE  1024
char buf[BUFSIZE];

static ssize_t Mywrite(struct file *fileptr, const char __user *ubuf, size_t buffer_len, loff_t *offset){
    /* Do nothing */
	return 0;
}

// read thread information (/proc/Mythread_info)
static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    int len = 0;

    // thread info data structure
    struct task_struct *t;

    // call Myread many time
    // prevent repeat, offset = 0 return data, offset > 0 return EOF
    if (*offset > 0)
        return 0;

    // list all thread (skip main thread)
    for_each_thread(current, t) {
    if (t->pid == current->pid) 
        continue;

    // write/output thread info into buffer
    len += scnprintf(buf + len, BUFSIZE - len,
                    "PID: %d, TID: %d, Priority: %d, State: %u\n",
                        current->pid,       
                        t->pid,           
                        t->prio,   
                        (unsigned int)t->__state);  
    }

    // write main thread itself if no other thread
    if (len == 0) {
        len += scnprintf(buf + len, BUFSIZE - len,
                    "PID: %d, TID: %d, Priority: %d, State: %u\n",
                        current->pid,
                        current->pid,
                        current->prio,
                        (unsigned int)current->__state);
    }

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
    proc_create(procfs_name, 0644, NULL, &Myops);   
    pr_info("My kernel says Hi");
    return 0;
}

static void My_Kernel_Exit(void){
    pr_info("My kernel says GOODBYE");
}

module_init(My_Kernel_Init);
module_exit(My_Kernel_Exit);

MODULE_LICENSE("GPL");
