#include "pcb.h"

void
new_sem(Semaphore *sem, int value) {
    sem->count = value;
    list_init(&sem->queue);
}

void
P(Semaphore *sem) {
    lock();
    sem->count --;
    if (sem->count < 0) {
        list_add_before(&sem->queue, &current->semq);
        sleep(); // 令当前进程立即进入睡眠
    }
    unlock();
}

void
V(Semaphore *sem) {
    lock();
    sem->count ++;
    if (sem->count <= 0) {
        assert(!list_empty(&sem->queue));
        PCB *pcb = list_entry(sem->queue.next, PCB, semq);
        list_del(sem->queue.next);
        wakeup(pcb); // 唤醒PCB所对应的进程
    }
    unlock();
}
