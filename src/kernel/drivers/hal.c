#include "hal.h"
#include "pcb.h"
#include "kernel.h"
#include "debug.h"
#include "string.h"

#define NR_DEV 64

static Device dev_pool[NR_DEV];
static ListHead free, devices;

//初始化设备池：以free为头节点，将未用的设备空间串接起来。
void init_hal(void){
	int i = 0;
	list_init(&free);
	for (i = 0; i < NR_DEV; i ++) {
		list_add_before(&free, &dev_pool[i].list);
	}
	list_init(&devices);
}
//注册设备：从free链表中取第一个设备空间，写上设备名称等后，插入到devices链表中
void hal_register(const char *name, pid_t pid, int dev_id) {
	lock();
	if (list_empty(&free)) {
		panic("no room for more device");
	}
	Device *dev = list_entry(free.next, Device, list);
	list_del(&dev->list);
	dev->name = name;
	dev->pid = pid;
	dev->dev_id = dev_id;
	list_add_before(&devices, &dev->list);
	unlock();
}
//列出设备清单：将devices中所有注册的设备依次显示出来
void hal_list(void) {
	lock();
	ListHead *it;
	printk("listing all registed devices:\n");
	list_foreach(it, &devices) {
		Device *dev = list_entry(it, Device, list);
		printk("%s #%d, #%d\n", dev->name, dev->pid, dev->dev_id);
	}
	unlock();
}
//获取设备：返回名字为name的设备
Device *hal_get(const char *name) {
	lock();
	ListHead *it;
	list_foreach(it, &devices) {
		Device *dev = list_entry(it, Device, list);
		if (strcmp(dev->name, name) == 0) {
			unlock();
			return dev;
		}
	}
	unlock();
	return NULL;
}
//向设备所在线程发送设备读/写消息，并从线程接收消息
static size_t
dev_rw(int type, Device *dev, off_t offset, void *buf, size_t count) {
	DevMessage m;
	m.header.type = type;
	m.dev_id = dev->dev_id;
	m.offset = offset;
	m.buf = buf;
	m.count = count;
	send(dev->pid, (Message*)&m);
	receive(dev->pid, (Message*)&m);
	return m.header.type;
}
//偏移offset位，读buf的count个字符
size_t
dev_read(Device *dev, off_t offset, void *buf, size_t count) {
	return dev_rw(MSG_DEVRD, dev, offset, buf, count);
}

size_t
dev_write(Device *dev, off_t offset, void *buf, size_t count) {
	return dev_rw(MSG_DEVWR, dev, offset, buf, count);
}

