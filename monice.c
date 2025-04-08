#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/pagewalk.h>
#include <linux/rcupdate.h>
#define NR_NODE 2


MODULE_LICENSE("GPL");


int count_folios[NR_NODE] = {0};

typedef int (*walk_page_range_t)(struct mm_struct *mm, unsigned long start, unsigned long end, const struct mm_walk_ops *ops, void *private);
static struct kprobe kp = { .symbol_name = "walk_page_range" };
static walk_page_range_t orig_walk_page_range;

static int my_pte_handler(pte_t *pte, unsigned long addr,
                         unsigned long next, struct mm_walk *walk)
{
    struct folio *folio = page_folio(pte_page(*pte));
    if (folio && !folio_test_slab(folio) && folio_pfn(folio)) {
        //printk(KERN_INFO "Folio PFN: %lu nid: %d\n", folio_pfn(folio), folio_nid(folio));
	count_folios[folio_nid(folio)] += 1;
    }
    return 0;
}

static const struct mm_walk_ops my_walk_ops = {
    .pte_entry = my_pte_handler,
};

void walk_task_folios(struct task_struct *task)
{
    struct mm_struct *mm = task->mm;
    if (!mm) return;

    mmap_read_lock(mm);
    orig_walk_page_range(mm, 0, TASK_SIZE, &my_walk_ops, NULL);
    mmap_read_unlock(mm);
}

static struct timer_list my_timer;

static void timer_callback(struct timer_list *t)
{
    struct task_struct *task;

    rcu_read_lock();
    for_each_process(task) {
        if (task->real_parent && task->real_parent->pid > 2) {
            int nice_val = PRIO_TO_NICE(task->static_prio);
            printk(KERN_INFO "PID: %d | PPID: %d | Nice: %d\n", task->pid, task->real_parent->pid, nice_val);
	    for (int i=0; i<NR_NODE; i++) {
	    	count_folios[i] = 0;
	    }
            walk_task_folios(task);
	    for (int i=0; i<NR_NODE; i++) {
	    	printk(KERN_INFO "PID:%d node:%d, count:%d\n", task->pid, i, count_folios[i]);
	    }
        }
    }
    rcu_read_unlock();
    printk(KERN_INFO "scan done\n");

    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
}

static int __init combined_init(void)
{
    if (register_kprobe(&kp) < 0) {
        printk(KERN_ERR "Kprobe registration failed\n");
        return -EINVAL;
    }
    orig_walk_page_range = (walk_page_range_t)kp.addr;
    unregister_kprobe(&kp);

    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
    
    printk(KERN_INFO "Monice loaded\n");
    return 0;
}

static void __exit combined_exit(void)
{
    del_timer_sync(&my_timer);
    printk(KERN_INFO "Monice unloaded\n");
}

module_init(combined_init);
module_exit(combined_exit);

