#include "string.h"
#include "pcb.h"
#include "kernel.h"

void copy_message(Message* dst,Message* src)
{
	dst->src=src->src;
	dst->dest=src->dest;
	dst->type=src->type;
	memcpy(dst->payload,src->payload,MSG_SZ);
	//printk("copying from %d to %d!\n",src->src,src->dest);
}

void send(pid_t dst, Message *m)
{
	lock();
	PCB *dst_pcb;
	if (src_irq == TRUE)//为即将发送的消息填写发送源进程号/中断
		m->src = MSG_HWINTR;
	else
		m->src = current->pid;
	m->dest = dst;//为即将发送的消息填写目标进程号
	dst_pcb = get_pcb(dst);
	//if(dst_pcb->message.count == MSG_NUM)
	if(list_empty(&dst_pcb->msg->freeq))	
	{
		char* str = "mail is full";
		panic(str);
	}	
	else	
	{	//printk("sending from %d to %d!\n",m->src,dst);
		ListHead* lh = &dst_pcb->msg->freeq;
		lh = lh -> next;
		list_del(lh);
		list_add_before(&dst_pcb->msg->getq, lh);
		copy_message(list_entry(lh,Message,getq),m);
		if(dst_pcb->receive == m->src)//如果目标进程正在等待这个消息
			V(&dst_pcb->message);
	}
	unlock();
}

void receive(pid_t dst, Message *m)
{
	lock();
	current->receive = dst;//当前进程需要接收一个来自dst的消息
	ListHead* lh = &current->msg->getq;
	if(dst == ANY)
	{	//printk("receive %d!\n",ANY);
		if(list_empty(&current->msg->getq))//信箱为空，则阻塞，否则复制消息
			P(&current->message);
		lh = lh -> next;		
		copy_message(m,list_entry(lh,Message,getq));
		list_del(lh);
		list_add_before(&current->msg->freeq,lh);//删除信箱中已接收好的消息
		current->receive = 0;//消息接收完成，不再需要接收消息
	}
	else
	{	//printk("receive %d!\n",dst);
		list_foreach(lh, &current->msg->getq)//查找信箱中是否存在来自dst的消息，若有则复制
		{
			if(dst == list_entry(lh,Message,getq)->src)
			{
				copy_message(m,list_entry(lh,Message,getq));
				list_del(lh);
				list_add_before(&current->msg->freeq,lh);
				current->receive = 0; break;
			}		
		}
		if(current->receive != 0)//若信箱中无来自dst的消息，则阻塞进程
		{	//printk("not receive from %d!,and pcb is %d,want to receive %d!\n",dst,current->pid,current->receive);
			P(&current->message);
			list_foreach(lh, &current->msg->getq)//再次查找信箱，将来自dst的消息复制
			{
				if(dst == list_entry(lh,Message,getq)->src)
				{
					copy_message(m,list_entry(lh,Message,getq));
					list_del(lh);
					list_add_before(&current->msg->freeq,lh);
					current->receive = 0; break;
				}		
			}
		}
	}
	unlock();
}



