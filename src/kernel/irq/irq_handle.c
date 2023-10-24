#include "kernel.h"
#include "pcb.h"
#include "irq.h"

static void (*handle[IRQ_SUM][HANDLE_SUM])(void);
static int handle_count[IRQ_SUM]= {0};

void add_irq_handle(int irq, void (*function)(void))
{
	handle[irq][handle_count[irq]] = function;
	handle_count[irq]++;
}

void run_irq_handle(int irq)
{
	int i;
	for(i=0; i<handle_count[irq]; i++)
		handle[irq][i]();
}

void irq_handle(TrapFrame *tf) {
	src_irq = TRUE;	
	int irq = tf->irq;
	assert(irq >= 0);
//	if(tf->irq<0)return;
	if (irq < 1000) {
		if(irq == 0x80)//如果是手动异常，切换进程
		{	//printk("irq_handle 0x80!\n");
			current->tf = tf;
			current = next_process();//切换到下一个线程 
		}
	} 
	else
	{	
		//printk("irq_handle %d!\n",irq);
		run_irq_handle(irq-1000);
		if(irq == 1000 || irq == 0x80)//如果是时钟中断，切换进程
		{
			current->tf = tf;
			current = next_process();//切换到下一个线程 
		}
	}
	src_irq = FALSE;
}

