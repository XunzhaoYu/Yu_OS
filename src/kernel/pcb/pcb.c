#include "pcb.h"
#include "kernel.h"
boolean src_irq = FALSE;
PCB pcb_pool[PCB_NUM];	

PCB *next_process()
{	
	//当除了idle没有其它线程时，执行idle线程
	if (list_empty(&idle->runq))
	{	
		return idle; 
	}	
	else//否则执行下一个可用进程
	{	
		PCB *now;
		now = list_entry(current->runq.next,PCB,runq);
	/*	ListHead* lh = &(current->runq);
		lh = lh -> next;
		now = list_entry(lh,PCB,runq);*/
		if(now == idle)
			now=list_entry(current->runq.next,PCB,runq);
		return now;
	}
}

PCB *get_pcb(pid_t pid)
{
	return &pcb_pool[pid];
}

PCB *init_thread(void *entry)
{
	assert(current == NULL);
	idle = pcb_pool;  
        //由于第一个线程idle一旦创建就不会消失，所以以它作为PCB链表的头结点
	list_init(&idle->runq);  
        list_init(&idle->freeq);   
	list_init(&idle->unusedq); 
	list_init(&idle->semq); 
	struct PCB *new; 
	int i;
        for(i = 1; i<PCB_NUM ; i++)   
        {   
        	new = &pcb_pool[i];
		new->pid = i; //为每个pcb设定唯一的进程号    
                list_add_before(&(idle->unusedq), &(new->unusedq));   
    	} 
	//以下是对idle的初始化
	idle->pid = 0;
	TrapFrame *tf = ((TrapFrame*)(idle->kstack + STK_SZ)) - 1;
	idle->tf = tf;
	tf->eax = 0;
	tf->ebx = 0;
	tf->ecx = 0;
	tf->edx = 0;
	tf->esi = 0;
	tf->edi = 0;
	tf->ebp = 0;
	tf->esp_ = 0;
	tf->ds = KSEL(SEG_KDATA);
	tf->es = KSEL(SEG_KDATA); 
	tf->fs = 0;
	tf->gs = 0;
	tf->cs = KSEL(SEG_KCODE);
	tf->eip = (int)entry;
	tf->eflags = FL_IF;
	idle->lock_count = 0;
	return idle;
}

PCB *create_kthread(void *entry)
{
	ListHead* lh = &(idle->unusedq);
	lh = lh -> next;
	PCB* pcb = list_entry(lh,PCB,unusedq);
	list_del(lh);
	TrapFrame *tf = ((TrapFrame*)(pcb->kstack + STK_SZ)) - 1;
	pcb->tf = tf;
	tf->eax = 0;
	tf->ebx = 0;
	tf->ecx = 0;
	tf->edx = 0;
	tf->esi = 0;
	tf->edi = 0;
	tf->ebp = 0;
	tf->esp_ = 0;
	tf->ds = KSEL(SEG_KDATA);
	tf->es = KSEL(SEG_KDATA); 
	tf->fs = 0;
	tf->gs = 0;
	tf->cs = KSEL(SEG_KCODE);
	tf->eip = (int)entry;
	tf->eflags = FL_IF;
	list_add_before(&(idle->freeq), lh);
	pcb->lock_count = 0;
	new_sem(&pcb->message,0);       //为新建的线程设置消息信号量
	pcb->receive = 0;
	//pcb->msg_count = 0;             //此时信箱中是空的
	int i;	
	Message *msg_new;
	list_init(&pcb->msg->getq);  
        list_init(&pcb->msg->freeq);   	
	for(i = 0; i<MSG_NUM ; i++)   
        {   
        	msg_new = &pcb->msg_pool[i];
                list_add_before(&pcb->msg->freeq, &msg_new->freeq);   
    	} 
	assert(list_empty(&pcb->msg->getq) == 1);
	assert(list_empty(&pcb->msg->freeq) == 0);
	return pcb;
}

void sleep(void)
{
	lock();
	list_del(&(current->runq));
	//list_add_before(&(idle->freeq), &(current->freeq));
	unlock();
	asm volatile("int $0x80");
}

void wakeup(PCB *pcb)
{
	lock();
	list_del(&(pcb->freeq));
	list_add_before(&(idle->runq), &(pcb->runq));
	unlock();	
}














