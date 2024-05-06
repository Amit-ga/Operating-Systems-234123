#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>


asmlinkage long sys_hello(void) {
    printk("Hello, World!\n");
    return 0;
}

/// <summary>
/// System call number: 334.
///Sets the weight of the current process.
/// </summary>
/// <param name="weight"> a non-negative number representing the user-specified process weight</param>
/// <returns>0 On success, -EINVAL if the specified weight is negative</returns>
asmlinkage long sys_set_weight(int weight) 
{
    if (weight >= 0) 
    {
        current->weight = weight;
        return 0;
    }
    return -EINVAL;
}

/// <summary>
/// System call number: 335.
/// Returns the weight of the current process.
/// </summary>
/// <param name=""></param>
/// <returns>the weight of the current process.</returns>
asmlinkage long sys_get_weight(void) 
{
    return current->weight;
}

/// <summary>
/// System call number: 336
/// Returns the sum of the weights of all the current process’ ancestors, starting from itself
/// </summary>
/// <param name=""></param>
/// <returns>the sum of weights of the ancestors</returns>
asmlinkage long sys_get_ancestor_sum(void) 
{
    long weights_sum = 0;
    struct task_struct* iter = current;
    while (iter->pid != 0) 
    {
        weights_sum += iter->weight;
        iter = iter->real_parent;
    }
    return weights_sum;
}

/// <summary>
/// a helper function for sys_get_heaviest_descendant
/// </summary>
/// <param name="root_task"></param>
/// <returns></returns>
long get_heaviest_descendant_helper(struct task_struct *task, long *max_weight) {
    struct task_struct *child;
    long heaviest_pid = -1;
    long descendant_pid = 0;
    // Traverse the children of the current task
    list_for_each_entry(child, &task->children, sibling) {
        if (child->weight == *max_weight && heaviest_pid > child->pid){
            heaviest_pid = child->pid;
        }
        else if (child->weight > *max_weight) {
            *max_weight = child->weight;
            heaviest_pid = child->pid;
        }

        // Recursively process the descendants of each child
        descendant_pid = get_heaviest_descendant_helper(child, max_weight);
        if (descendant_pid != -1)
            heaviest_pid = descendant_pid;
    }

    return heaviest_pid;
}

/// <summary>
/// System call number: 337
/// Returns the pid  of the descendant of the current process with the heaviest weight among all the current process’s descendants.
/// If there are 2 descendants with the same maximal weight, return the one with the smallest pid.
/// </summary>
/// <param name=""></param>
/// <returns>pid of the descendant with the heaviest weight on success,
/// -ECHILD if the current process does not currently have any children</returns>
asmlinkage long sys_get_heaviest_descendant(void) 
{
    long max_weight = -1;
    if (!list_empty(&current->children)) 
    {
        // Traverse the descendants of the current process
        long heaviest_pid = get_heaviest_descendant_helper(current, &max_weight);  
        return heaviest_pid;  
    }
    return -ECHILD;
}