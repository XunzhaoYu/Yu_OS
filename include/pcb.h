#include "adt/list.h"
#include "x86.h"
#include "message.h"
#include "semaphore.h"

struct PCB {
	TrapFrame *tf;
	pid_t pid;//进程号
	int lock_count;//嵌套上锁数
	ListHead runq,freeq,semq,unusedq;//各个线程队列的链表
    	char kstack[STK_SZ];//线程的堆栈
	Message msg_pool[MSG_NUM];//信箱
	Semaphore message;//消息信号量
	Message *msg;//信箱中消息队列的头指针
	pid_t receive;//线程阻塞时标记所等待的消息源的线程号
	//int msg_count;//信箱中的当前消息位置
};
typedef struct PCB PCB;

PCB *current ;
PCB *idle;
PCB *next_process();
PCB *get_pcb(pid_t pid);
PCB *init_thread(void *entry);
PCB *create_kthread(void *entry);
extern boolean src_irq;

static inline void lock() 
{
	cli();
	current->lock_count ++;
}

static inline void unlock()
{
	current->lock_count --; 
	assert(current->lock_count >= 0);
	if (current->lock_count == 0) 
		sti();
}
void sleep(void);
void wakeup(PCB *pcb);
