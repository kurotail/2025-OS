#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <asm/current.h>

#define procfs_name "Mythread_info"
#define BUFSIZE  1024
char buf[BUFSIZE]; //kernel buffer

static ssize_t Mywrite(struct file *fileptr, const char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    int i;
    int tid = current->pid;
    int slot_size = 256;
    int found_slot = -1;
    int write_size = buffer_len < (slot_size - 4) ? buffer_len : (slot_size - 4);
    int *slot_tid;

    for (i = 0; i < 4; i++) {
        slot_tid = (int *)&buf[i * slot_size];
        if (*slot_tid == tid) {
            found_slot = i;
            break;
        }
        if (*slot_tid == 0 && found_slot == -1) {
            found_slot = i;
        }
    }

    if (found_slot != -1) {
        int *slot_tid = (int *)&buf[found_slot * slot_size];
        *slot_tid = tid;
        memset(&buf[found_slot * slot_size + 4], 0, slot_size - 4);
        if (copy_from_user(&buf[found_slot * slot_size + 4], ubuf, write_size)) return -EFAULT;
        for (;buf[found_slot * slot_size + 3 + write_size] == 0; write_size--);
        i = snprintf(&buf[found_slot * slot_size + 4 + write_size], slot_size-4-write_size, 
            "\nPID: %d, TID: %d, time: %lld\n",
            current->tgid, tid, current->utime/100/1000
        );
        buf[found_slot * slot_size + 4 + write_size + i] = 0;
    }
    else return 0;

    return buffer_len;
    /****************/
}


static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    int i;
    int tid = current->pid;
    int slot_size = 256;
    int found_slot = -1;
    if (*offset > 0) return 0;
    for (i = 0; i < 4; i++) {
        int *slot_tid = (int *)&buf[i * slot_size];
        if (*slot_tid == tid) {
            found_slot = i;
            break;
        }
    }
    if (found_slot == -1) return 0;
    
    if (copy_to_user(ubuf, &buf[i * slot_size +4], slot_size-4)) return -EFAULT;
    memset(&buf[found_slot*slot_size], 0, slot_size);
    *offset = slot_size-4;
    return slot_size-4;
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
    remove_proc_entry(procfs_name, NULL);
    pr_info("My kernel says GOODBYE");
}

module_init(My_Kernel_Init);
module_exit(My_Kernel_Exit);

MODULE_LICENSE("GPL");
