#include "kernel.h"
#include "vm.h"
#include "irq.h"
#include "tty.h"
#include "time.h"
#define NBUF 5
#define INTR assert(readf()&FL_IF)
#define NOINTR assert(~readf()&FL_IF)

static int pid1,pid2,pid3;
void test_pcb1()
{
	//printk("now is in pcb%d\n",pid1);
	Message m;
	send(pid2,&m);
	printk("#%d send: %d\n",pid1,pid2);
	while(TRUE)
	{	
		Message m3;
		receive(pid3,&m3);
		printk("#%d receive: %d\n",pid1,pid3);
		Message m1;
		send(pid2,&m1);
		printk("#%d send: %d\n",pid1,pid2);
	}
}
void test_pcb2()
{
	//printk("now is in pcb%d\n",pid2);
	while(TRUE)
	{
		Message m1;
		receive(pid1,&m1);
		printk("#%d receive: %d\n",pid2,pid1);
		Message m2;
		send(pid3,&m2);
		printk("#%d send: %d\n",pid2,pid3);
	}
}
void test_pcb3()
{
	//printk("now is in pcb%d\n",pid3);
	while(TRUE)
	{	
		Message m2;
		receive(pid2,&m2);
		printk("#%d receive: %d\n",pid3,pid2);
		Message m3;
		send(pid1,&m3);
		printk("#%d send: %d\n",pid3,pid1);
	}
}
void my_test()
{
	PCB *pcb1=create_kthread(test_pcb1);
	PCB *pcb2=create_kthread(test_pcb2);
	PCB *pcb3=create_kthread(test_pcb3);
	pid2=pcb2->pid;
	pid1=pcb1->pid;	
	pid3=pcb3->pid;
	wakeup(pcb1);
	wakeup(pcb2);
	wakeup(pcb3);
	//wakeup(create_kthread(test_pcb3));		
}

void
echo() {
	static int tty = 1;
	char name[] = "tty*", buf[256];
	Device *dev;
	lock();
	name[3] = '0' + (tty ++);
	unlock();
	while (1) {
		dev = hal_get(name);
		if (dev != NULL) {
			dev_write(dev, 0, name, 4);
			//dev_write(dev, 0, "# ", 2);
			int i, nread = dev_read(dev, 0, buf, 255);
			buf[nread] = 0;
			for (i = 0; i < nread; i ++) {
				if (buf[i] >= 'a' && buf[i] <= 'z') {
					buf[i] += 'A' - 'a';
				}
			}
			dev_write(dev, 0, "Got: ", 5);
			dev_write(dev, 0, buf, nread);
			dev_write(dev, 0, "\n", 1);
		} else {
			printk("%s\n", name);
		}
	}
}

void
test() {
	printk("src_irq: %d!\n",src_irq);
	int i;
	for (i = 0; i < NR_TTY; i ++) {
		wakeup(create_kthread(echo));
	}
}

void
os_init(void) {
	init_seg();
	init_debug();
	init_idt();
	init_i8259();
	current = NULL;
	printk("The OS is now working!\n");
	assert(current == NULL);
	current = init_thread(os_init);//设置首个线程为idle
	assert(current != NULL);
	printk("thread init is finished!\n");
	init_hal();
	init_tty();
	init_timer();
	PCB *pcb_ttyd=create_kthread(ttyd);
	TTY=pcb_ttyd->pid;
	wakeup(pcb_ttyd);
	sti();
	//my_test();
	test();
	while (TRUE) {
		wait_intr();
	}
}

void
entry(void) {
	init_kvm();
	void(*volatile next)(void) = os_init;
	asm volatile("addl %0, %%esp" : : ""(KOFFSET));
	next();
	panic("init code should never return");
}
